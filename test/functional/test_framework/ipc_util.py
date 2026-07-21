#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Shared utilities for IPC (multiprocess) interface tests."""
import asyncio
import inspect
from contextlib import asynccontextmanager
from dataclasses import dataclass
from io import BytesIO
from pathlib import Path
import shutil
import sys
from typing import Optional

if sys.platform == 'win32':
    import ctypes as _ctypes
    import os as _os
    import socket as _socket

    # Windows 10 1803+ supports AF_UNIX via Winsock, but Python's asyncio
    # documents create_unix_connection() as Unix-only; both SelectorEventLoop and
    # ProactorEventLoop raise NotImplementedError. Implement it manually by patching
    # ProactorEventLoop (the default Windows asyncio event loop since Python 3.8).
    #
    # Why ProactorEventLoop and not SelectorEventLoop: SelectorEventLoop uses
    # select(), but Winsock requires all sockets in one select() call to belong to
    # the same service provider. The event loop always adds an internal AF_INET
    # wakeup socket, so mixing it with our AF_UNIX socket violates that restriction
    # and produces WSAEINVAL. ProactorEventLoop instead registers each socket
    # independently with an IOCP completion port and uses overlapped WSARecv, which
    # has no such restriction.
    #
    # Why ctypes for connect(): Python's socket module C code uses getsockaddrarg()
    # to parse addresses, switching on the socket family. AF_UNIX support is compiled
    # in only when socket.AF_UNIX is defined. The Python build in CI's
    # hostedtoolcache does not expose socket.AF_UNIX even though the OS supports it,
    # so getsockaddrarg() falls through to the "bad family" default. Calling
    # ws2_32.connect() via ctypes bypasses this check entirely.
    #
    # Note: ws2_32.connect() is a blocking call. For a local AF_UNIX socket the
    # kernel returns immediately, so blocking the event loop thread is acceptable
    # here. A future improvement could run just the connect step in a thread pool
    # worker via loop.run_in_executor() if this ever needs to handle slow paths.

    class _SOCKADDR_UN(_ctypes.Structure):
        _fields_ = [("sun_family", _ctypes.c_ushort), ("sun_path", _ctypes.c_char * 108)]

    _ws2_32 = _ctypes.windll.ws2_32
    # SOCKET is UINT_PTR (pointer-sized) on Windows; use c_size_t so the handle
    # is not truncated to 32 bits on 64-bit processes.
    _ws2_32.connect.argtypes = [_ctypes.c_size_t, _ctypes.POINTER(_SOCKADDR_UN), _ctypes.c_int]
    _ws2_32.connect.restype = _ctypes.c_int
    _ws2_32.WSAGetLastError.restype = _ctypes.c_int

    async def _win32_create_unix_connection(self, protocol_factory, path=None, *,
                                            ssl=None, sock=None,
                                            server_hostname=None,
                                            ssl_handshake_timeout=None,
                                            ssl_shutdown_timeout=None):
        if (path is None) == (sock is None):
            raise ValueError("exactly one of path and sock must be specified")
        if sock is None:
            # AF_UNIX may not be exposed in Python's socket module on Windows even
            # when the OS supports it (depends on SDK version at Python compile time).
            # Fall back to the raw integer constant 1, which is AF_UNIX on all platforms;
            # socket.socket() passes the value through to winsock.socket() directly.
            _af_unix = getattr(_socket, 'AF_UNIX', 1)
            sock = _socket.socket(_af_unix, _socket.SOCK_STREAM)
            try:
                # os.fsencode(os.fspath()) handles str, bytes, and PathLike correctly.
                # str(path).encode() would mangle bytes paths as "b'...'" strings.
                _path_bytes = _os.fsencode(_os.fspath(path))
                if b"\x00" in _path_bytes:
                    raise ValueError("AF_UNIX path must not contain a NUL byte")
                if len(_path_bytes) >= 108:
                    raise ValueError(f"AF_UNIX path too long ({len(_path_bytes)} >= 108 bytes)")
                _addr = _SOCKADDR_UN(sun_family=1, sun_path=_path_bytes)
                _ret = _ws2_32.connect(_ctypes.c_size_t(sock.fileno()),
                                       _ctypes.byref(_addr), _ctypes.sizeof(_addr))
                if _ret != 0:
                    _err = _ws2_32.WSAGetLastError()
                    raise OSError(_err, f"AF_UNIX connect() failed (WSA error {_err})")
            except:
                sock.close()
                raise
        return await self.create_connection(protocol_factory, sock=sock, ssl=ssl,
                                            server_hostname=server_hostname,
                                            ssl_handshake_timeout=ssl_handshake_timeout,
                                            ssl_shutdown_timeout=ssl_shutdown_timeout)

    # Patch the concrete ProactorEventLoop class (public API, not the private base)
    # so every event loop instance created by asyncio.run() gets the patched method.
    asyncio.ProactorEventLoop.create_unix_connection = _win32_create_unix_connection

from test_framework.messages import CBlock
from test_framework.util import (
    assert_equal
)

# Test may be skipped and not have capnp installed
try:
    import capnp  # type: ignore[import] # noqa: F401
except ModuleNotFoundError:
    pass


# Stores the result of getCoinbaseTx()
@dataclass
class CoinbaseTxData:
    version: int
    sequence: int
    scriptSigPrefix: bytes
    witness: Optional[bytes]
    blockRewardRemaining: int
    requiredOutputs: list[bytes]
    lockTime: int


@asynccontextmanager
async def destroying(obj, ctx):
    """Call obj.destroy(ctx) at end of with: block. Similar to contextlib.closing."""
    try:
        yield obj
    finally:
        await obj.destroy(ctx)


async def wait_and_do(wait_fn, do_fn):
    """Call wait_fn, then sleep, then call do_fn in a parallel task. Wait for
    both tasks to complete."""
    wait_started = asyncio.Event()
    result = None

    async def wait():
        nonlocal result
        wait_started.set()
        result = await wait_fn

    async def do():
        await wait_started.wait()
        await asyncio.sleep(0.1)
        # Let do_fn be either a callable or an awaitable object
        if inspect.isawaitable(do_fn):
            await do_fn
        else:
            do_fn()

    await asyncio.gather(wait(), do())
    return result


def load_capnp_modules(config):
    if capnp_bin := shutil.which("capnp"):
        # Add the system cap'nproto path so include/capnp/c++.capnp can be found.
        capnp_dir = Path(capnp_bin).resolve().parent.parent / "include"
    else:
        # If there is no system cap'nproto, the pycapnp module should have its own "bundled"
        # includes at this location. If pycapnp was installed with bundled capnp,
        # capnp/c++.capnp can be found here.
        capnp_dir = Path(capnp.__path__[0]).parent
    src_dir = Path(config['environment']['SRCDIR']) / "src"
    mp_dir = src_dir / "ipc" / "libmultiprocess" / "include"
    # List of import directories. Note: it is important for mp_dir to be
    # listed first, in case there are other libmultiprocess installations on
    # the system, to ensure that `import "/mp/proxy.capnp"` lines load the
    # same file as capnp.load() loads directly below, and there are not
    # "failed: Duplicate ID @0xcc316e3f71a040fb" errors.
    imports = [str(mp_dir), str(capnp_dir), str(src_dir)]
    return {
        "proxy": capnp.load(str(mp_dir / "mp" / "proxy.capnp"), imports=imports),
        "init": capnp.load(str(src_dir / "ipc" / "capnp" / "init.capnp"), imports=imports),
        "echo": capnp.load(str(src_dir / "ipc" / "capnp" / "echo.capnp"), imports=imports),
        "mining": capnp.load(str(src_dir / "ipc" / "capnp" / "mining.capnp"), imports=imports),
    }


async def make_capnp_init_ctx(self, node_index=0):
    node = self.nodes[node_index]
    # Establish a connection, and create Init proxy object.
    connection = await capnp.AsyncIoStream.create_unix_connection(node.ipc_socket_path)
    client = capnp.TwoPartyClient(connection)
    init = client.bootstrap().cast_as(self.capnp_modules['init'].Init)
    # Create a remote thread on the server for the IPC calls to be executed in.
    threadmap = init.construct().threadMap
    thread = threadmap.makeThread("pythread").result
    ctx = self.capnp_modules['proxy'].Context()
    ctx.thread = thread
    # Return both.
    return ctx, init


async def mining_create_block_template(mining, stack, ctx, *args, **kwargs):
    """Call mining.createNewBlock() and return template, then call template.destroy() when stack exits."""
    response = await mining.createNewBlock(ctx, *args, **kwargs)
    if not response._has("result"):
        return None
    return await stack.enter_async_context(destroying(response.result, ctx))


async def mining_wait_next_template(template, stack, ctx, opts):
    """Call template.waitNext() and return template, then call template.destroy() when stack exits."""
    response = await template.waitNext(ctx, opts)
    if not response._has("result"):
        return None
    return await stack.enter_async_context(destroying(response.result, ctx))


async def mining_get_block(block_template, ctx):
    block_data = BytesIO((await block_template.getBlock(ctx)).result)
    block = CBlock()
    block.deserialize(block_data)
    return block


async def mining_get_coinbase_tx(block_template, ctx) -> CoinbaseTxData:
    assert block_template is not None
    # Note: the template_capnp struct will be garbage-collected when this
    # method returns, so it is important to copy any Data fields from it
    # which need to be accessed later using the bytes() cast. Starting with
    # pycapnp v2.2.0, Data fields have type `memoryview` and are ephemeral.
    template_capnp = (await block_template.getCoinbaseTx(ctx)).result
    witness: Optional[bytes] = None
    if template_capnp._has("witness"):
        witness = bytes(template_capnp.witness)
    return CoinbaseTxData(
        version=int(template_capnp.version),
        sequence=int(template_capnp.sequence),
        scriptSigPrefix=bytes(template_capnp.scriptSigPrefix),
        witness=witness,
        blockRewardRemaining=int(template_capnp.blockRewardRemaining),
        requiredOutputs=[bytes(output) for output in template_capnp.requiredOutputs],
        lockTime=int(template_capnp.lockTime),
    )

async def make_mining_ctx(self, node_index=0):
    """Create IPC context and Mining proxy object."""
    ctx, init = await make_capnp_init_ctx(self, node_index)
    self.log.debug("Create Mining proxy object")
    mining = init.makeMining(ctx).result
    return ctx, mining

def assert_capnp_failed(e, description_prefix):
    assert e.description.startswith(description_prefix), f"Expected description starting with '{description_prefix}', got '{e.description}'"
    assert_equal(e.type, "FAILED")


async def assert_create_new_block_fails(ctx, mining, opts, expected_msg):
    """Assert that mining.createNewBlock fails with the expected remote exception."""
    try:
        await mining.createNewBlock(ctx, opts)
        raise AssertionError("createNewBlock unexpectedly succeeded")
    except capnp.lib.capnp.KjException as e:
        assert_capnp_failed(e, f"remote exception: std::exception: {expected_msg}")
