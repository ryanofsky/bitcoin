#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the IPC (multiprocess) interface."""
import asyncio
import functools
from io import BytesIO
from pathlib import Path
import shutil
import tempfile
from test_framework.messages import (CBlock, CTransaction, ser_uint256)
from test_framework.test_framework import BitcoinTestFramework

# Test may be skipped and not have capnp installed
try:
    import capnp  # type: ignore[import] # noqa: F401
except ImportError:
    pass


class IPCInterfaceTest(BitcoinTestFramework):

    def set_test_params(self):
        self.num_nodes = 1

    def skip_test_if_missing_module(self):
        self.skip_if_no_py_capnp()
        self.skip_if_no_ipc()

    def setup_nodes(self):
        self.extra_init = [{"ipcbind": True}]
        super().setup_nodes()

    @functools.cache
    def capnp_modules(self):
        capnp_dir = Path(shutil.which("capnp")).parent.parent / "include"
        src_dir = Path(self.config['environment']['SRCDIR']) / "src"
        mp_dir = src_dir / "ipc" / "libmultiprocess" / "include"
        imports = [str(capnp_dir), str(src_dir), str(mp_dir)]
        return {
            "proxy": capnp.load(str(mp_dir / "mp" / "proxy.capnp"), imports=imports),
            "init": capnp.load(str(src_dir / "ipc" / "capnp" / "init.capnp"), imports=imports),
            "echo": capnp.load(str(src_dir / "ipc" / "capnp" / "echo.capnp"), imports=imports),
            "mining": capnp.load(str(src_dir / "ipc" / "capnp" / "mining.capnp"), imports=imports),
        }

    async def make_capnp_init_ctx(self):
        node = self.nodes[0]
        modules = self.capnp_modules()
        # Establish a connection, and create Init proxy object.
        connection = await capnp.AsyncIoStream.create_unix_connection(node.ipc_socket_path)
        client = capnp.TwoPartyClient(connection)
        init = client.bootstrap().cast_as(modules['init'].Init)
        # Create a thread, and a context object it is registered in.
        threadmap = init.construct().threadMap
        thread = threadmap.makeThread("pythread").result
        ctx = modules['proxy'].Context()
        ctx.thread = thread
        # Return both.
        return init, ctx

    def run_echo_test(self):
        self.log.info("Running echo test")
        async def async_routine():
            init, ctx = await self.make_capnp_init_ctx()
            # Create Echo proxy object.
            echo = init.makeEcho(ctx).result
            # Test a few invocations of echo.
            for s in ["hallo", "", "haha"]:
                result_eval = (await echo.echo(ctx, s)).result
                assert s == result_eval
            # Destroy the object.
            echo.destroy(ctx)
        asyncio.run(capnp.run(async_routine()))

    def run_mining_test(self):
        self.log.info("Running mining test")
        modules = self.capnp_modules()
        async def async_routine():
            init, ctx = await self.make_capnp_init_ctx()
            # Create Mining proxy object.
            mining = init.makeMining(ctx)
            # Test simple inspectors.
            assert (await mining.result.isTestChain(ctx))
            assert (await mining.result.isInitialBlockDownload(ctx))
            blockref = await mining.result.getTip(ctx)
            assert blockref.hasResult
            assert len(blockref.result.hash) == 32
            assert blockref.result.height > 100
            # Mine a block.
            wait = mining.result.waitTipChanged(ctx, blockref.result.hash, 1000.0)
            self.generate(self.nodes[0], 1)
            newblockref = await wait
            assert len(newblockref.result.hash) == 32
            assert newblockref.result.hash != blockref.result.hash
            assert newblockref.result.height == blockref.result.height + 1
            # Wait for timeout (1000 milliseconds)
            wait = mining.result.waitTipChanged(ctx, newblockref.result.hash, 1000.0)
            oldblockref = await wait
            assert len(newblockref.result.hash) == 32
            assert oldblockref.result.hash == newblockref.result.hash
            assert oldblockref.result.height == newblockref.result.height

            # Create a template.
            opts = modules['mining'].BlockCreateOptions()
            opts.useMempool = True
            opts.blockReservedWeight = 4000
            opts.coinbaseOutputMaxAdditionalSigops = 0
            template = mining.result.createNewBlock(opts)
            # Test some inspectors of template.
            header = await template.result.getBlockHeader(ctx)
            assert len(header.result) == 80
            block_data = BytesIO((await template.result.getBlock(ctx)).result)
            block = CBlock()
            block.deserialize(block_data)
            assert ser_uint256(block.hashPrevBlock) == newblockref.result.hash
            assert len(block.vtx) >= 1
            txfees = await template.result.getTxFees(ctx)
            assert len(txfees.result) == 0
            txsigops = await template.result.getTxSigops(ctx)
            assert len(txsigops.result) == 0
            coinbase_data = BytesIO((await template.result.getCoinbaseTx(ctx)).result)
            coinbase = CTransaction()
            coinbase.deserialize(coinbase_data)
            assert coinbase.vin[0].prevout.hash == 0
            # Wait for a new template.
            waitoptions = modules['mining'].BlockWaitOptions()
            waitoptions.timeout = 1000.0
            waitnext = template.result.waitNext(ctx, waitoptions)
            new_tip = self.generate(self.nodes[0], 1)
            template2 = await waitnext
            assert template2
            block2_data = BytesIO((await template2.result.getBlock(ctx)).result)
            block2 = CBlock()
            block2.deserialize(block2_data)
            assert int(new_tip[0], 16) == block2.hashPrevBlock
            # Wait for another, but timeout.
            waitnext = template2.result.waitNext(ctx, waitoptions)
            template3 = await waitnext
            assert template3.to_dict() == {}
            # Destroy template objects
            template.result.destroy(ctx)
            template2.result.destroy(ctx)
        asyncio.run(capnp.run(async_routine()))

    def run_test(self):
        self.run_echo_test()
        self.run_mining_test()

if __name__ == '__main__':
    IPCInterfaceTest(__file__).main()
