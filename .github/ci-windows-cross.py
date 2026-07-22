#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or https://opensource.org/license/mit/.

import argparse
import os
import shlex
import subprocess
import sys
import time
from pathlib import Path

sys.path.append(str(Path(__file__).resolve().parent.parent / "test"))
from download_utils import download_script_assets


def run(cmd, **kwargs):
    print("+ " + shlex.join(cmd), flush=True)
    kwargs.setdefault("check", True)
    try:
        return subprocess.run(cmd, **kwargs)
    except Exception as e:
        sys.exit(str(e))


def print_version():
    bitcoind = Path.cwd() / "bin" / "bitcoind.exe"
    run([str(bitcoind), "-version"])


def check_manifests():
    release_dir = Path.cwd() / "bin"
    manifest_path = release_dir / "bitcoind.manifest"

    cmd_bitcoind_manifest = [
        "mt.exe",
        "-nologo",
        f"-inputresource:{release_dir / 'bitcoind.exe'}",
        f"-out:{manifest_path}",
    ]
    run(cmd_bitcoind_manifest)
    print(manifest_path.read_text())

    skipped = {  # Skip as they currently do not have manifests
        "fuzz.exe",
        "bench_bitcoin.exe",
    }
    for entry in release_dir.iterdir():
        if entry.suffix.lower() != ".exe":
            continue
        if entry.name in skipped:
            print(f"Skipping {entry.name} (no manifest present)")
            continue
        print(f"Checking {entry.name}")
        run(["mt.exe", "-nologo", f"-inputresource:{entry}", "-validate_manifest"])


def prepare_tests():
    workspace = Path.cwd()
    config_path = workspace / "test" / "config.ini"
    rpcauth_path = workspace / "share" / "rpcauth" / "rpcauth.py"
    replacements = {
        "SRCDIR=": f"SRCDIR={workspace}",
        "BUILDDIR=": f"BUILDDIR={workspace}",
        "RPCAUTH=": f"RPCAUTH={rpcauth_path}",
    }
    lines = config_path.read_text().splitlines()
    for index, line in enumerate(lines):
        for prefix, new_value in replacements.items():
            if line.startswith(prefix):
                lines[index] = new_value
                break
    content = "\n".join(lines) + "\n"
    config_path.write_text(content)
    print(content)
    run([sys.executable, "-m", "pip", "install", "pyzmq", "pycapnp"])

    dest = workspace / "unit_test_data"
    download_script_assets(dest)


def run_functional_tests():
    workspace = Path.cwd()
    num_procs = str(os.process_cpu_count())
    # Use a short tmpdir so IPC socket paths stay under UNIX_PATH_MAX (108 bytes).
    # The default workspace prefix (D:\a\bitcoin\bitcoin\_ _) + test_runner_₿_🏃_TIMESTAMP
    # + test name gives ~108+ bytes for longer test names like interface_ipc_mining.
    tmpdir = "D:\\t"
    Path(tmpdir).mkdir(exist_ok=True)
    test_runner_cmd = [
        sys.executable,
        str(workspace / "test" / "functional" / "test_runner.py"),
        "--jobs",
        num_procs,
        f"--tmpdirprefix={tmpdir}",
        "--combinedlogslen=99999999",
        *shlex.split(os.environ.get("TEST_RUNNER_EXTRA", "").strip()),
    ]
    run(test_runner_cmd)


def run_ipc_repro():
    # TEMP: standalone reproducer for the STATUS_HEAP_CORRUPTION (0xC0000374)
    # teardown crash seen in interface_ipc_cli.py / interface_ipc_mining.py on
    # msvcrt builds. Mirrors the interface_ipc_cli.py invocation (bitcoind with
    # -ipcbind=unix, bitcoin-cli with -rpcpassword=wrong so only the IPC path
    # can succeed) but runs bitcoin-cli under gdb in batch mode, so a crash
    # produces a symbolized (DWARF) backtrace directly in the CI log, which
    # WinDbg/cdb could not provide for MinGW binaries.
    workspace = Path.cwd()
    # bitcoin-node (not bitcoind) is the multiprocess binary accepting -ipcbind.
    node_exe = workspace / "bin" / "bitcoin-node.exe"
    cli = workspace / "bin" / "bitcoin-cli.exe"
    gdb = Path("C:/msys64/mingw64/bin/gdb.exe")
    if not gdb.exists():
        run(["C:/msys64/usr/bin/pacman.exe", "-Sy", "--noconfirm", "--needed", "mingw-w64-x86_64-gdb"])

    # Short datadir keeps the unix socket path well under UNIX_PATH_MAX.
    datadir = Path("D:/r")
    datadir.mkdir(exist_ok=True)
    node_log_path = datadir / "node-stdout.log"
    # IPC_DIAG_LOG makes the TEMP EventLoop/CapnpProtocol teardown diag lines in
    # the node process go to its debug.log (normally only bitcoin-cli sets it
    # for itself), so a node-side shutdown crash is localized too.
    node_env = {**os.environ, "IPC_DIAG_LOG": str(datadir / "regtest" / "debug.log")}
    # Run the node under gdb too: the interface_ipc_mining.py failure mode is
    # this heap corruption at node shutdown, so a node-side crash produces a
    # backtrace in node-stdout.log (printed below).
    node_cmd = [str(gdb), "-q", "-batch", "-return-child-result",
                "-ex", "run", "-ex", "bt", "-ex", "thread apply all bt", "--args",
                str(node_exe), "-regtest", f"-datadir={datadir}", "-ipcbind=unix",
                "-listen=0", "-connect=0", "-debug=ipc", "-loglevel=trace"]
    print("+ " + shlex.join(node_cmd), flush=True)
    with open(node_log_path, "w") as node_log:
        node = subprocess.Popen(node_cmd, stdout=node_log, stderr=subprocess.STDOUT, env=node_env)
    sock = datadir / "regtest" / "node.sock"
    for _ in range(300):
        if sock.exists():
            break
        if node.poll() is not None:
            print(node_log_path.read_text(errors="replace"))
            sys.exit(f"bitcoin-node exited early with code {node.returncode:#x}")
        time.sleep(0.2)
    else:
        sys.exit("timed out waiting for node.sock to appear")

    iterations = 50
    crashes = 0
    cli_cmd = [str(cli), "-regtest", f"-datadir={datadir}", "-rpcpassword=wrong", "echo", "foo"]
    gdb_cmd = [str(gdb), "-q", "-batch", "-return-child-result",
               "-ex", "run", "-ex", "bt", "-ex", "thread apply all bt",
               "--args"] + cli_cmd
    print("+ " + shlex.join(gdb_cmd), flush=True)
    for i in range(1, iterations + 1):
        try:
            result = subprocess.run(gdb_cmd, capture_output=True, text=True,
                                    errors="replace", timeout=120)
        except subprocess.TimeoutExpired as e:
            crashes += 1
            print(f"iteration {i}/{iterations}: TIMEOUT")
            print(e.stdout or "")
            continue
        crashed = result.returncode != 0 or "received signal" in result.stdout
        print(f"iteration {i}/{iterations}: {'CRASH' if crashed else 'ok'} "
              f"exit={result.returncode & 0xFFFFFFFF:#010x}", flush=True)
        if crashed:
            crashes += 1
            print(result.stdout)
            print(result.stderr, file=sys.stderr)

    # Show the IPC diag checkpoint lines (written by the TEMP instrumentation
    # in bitcoin-cli.cpp / protocol.cpp / mp proxy.cpp) for context.
    debug_log = datadir / "regtest" / "debug.log"
    if debug_log.exists():
        diag_lines = [line for line in debug_log.read_text(errors="replace").splitlines()
                      if "[ipc-cli]" in line or "[eventloop-" in line or "[capnp-dtor]" in line]
        print(f"diag lines from {debug_log} (last 100 of {len(diag_lines)}):")
        for line in diag_lines[-100:]:
            print(line)

    # Stop the node with cookie-auth HTTP RPC (no -rpcpassword override) and
    # report its exit code: the interface_ipc_mining.py failure mode is this
    # same heap corruption at node shutdown after serving IPC connections.
    subprocess.run([str(cli), "-regtest", f"-datadir={datadir}", "stop"], timeout=120, check=False)
    try:
        node.wait(timeout=120)
    except subprocess.TimeoutExpired:
        node.kill()
        node.wait(timeout=120)
    print(f"bitcoin-node exit code: {node.returncode & 0xFFFFFFFF:#010x}")
    node_out = node_log_path.read_text(errors="replace")
    if node.returncode != 0:
        print(node_out)
    else:
        print("tail of node-stdout.log:")
        print("\n".join(node_out.splitlines()[-20:]))
    if crashes:
        sys.exit(f"{crashes}/{iterations} bitcoin-cli iterations crashed")
    if node.returncode != 0:
        sys.exit(f"bitcoin-node exited with {node.returncode & 0xFFFFFFFF:#010x}")


def run_unit_tests():
    workspace = Path.cwd()
    os.environ["DIR_UNIT_TEST_DATA"] = str(workspace / "unit_test_data")
    # Can't use ctest here like other jobs as we don't have a CMake build tree.
    commands = [
        # Intentionally run sequentially here, to catch test case failures caused by dirty global state from prior test cases:
        ["./bin/test_bitcoin.exe", "-l", "test_suite"],
        ["./src/secp256k1/bin/exhaustive_tests.exe"],
        ["./src/secp256k1/bin/noverify_tests.exe"],
        ["./src/secp256k1/bin/tests.exe"],
        ["./src/univalue/object.exe"],
        ["./src/univalue/unitester.exe"],
    ]
    for cmd in commands:
        run(cmd)


def main():
    parser = argparse.ArgumentParser(description="Utility to run Windows CI steps.")
    steps = list(map(lambda f: f.__name__, [
        print_version,
        check_manifests,
        prepare_tests,
        run_ipc_repro,
        run_unit_tests,
        run_functional_tests,
    ]))
    parser.add_argument("step", choices=steps, help="CI step to perform.")
    args = parser.parse_args()

    os.environ.setdefault(
        "PREVIOUS_RELEASES_DIR",
        str(Path.cwd() / "previous_releases"),
    )

    exec(f'{args.step}()')


if __name__ == "__main__":
    main()
