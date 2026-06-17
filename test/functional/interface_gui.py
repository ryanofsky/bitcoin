#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that bitcoin-gui starts up and can be stopped via RPC."""

import platform

from test_framework.test_framework import (
    BitcoinTestFramework,
    SkipTest,
)


class GuiTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 1
        self.extra_args = [["-server"]]

    def skip_test_if_missing_module(self):
        self.skip_if_no_gui()
        # On Windows, bitcoin-gui.exe is a wrapper that execs bitcoin-qt but exits
        # immediately without waiting for bitcoin-qt to finish starting. The test
        # framework detects the wrapper exiting and raises FailedToStartError before
        # bitcoin-qt has a chance to open its RPC server.
        if platform.system() == "Windows":
            raise SkipTest("bitcoin-gui.exe wrapper exits before bitcoin-qt starts on Windows")
        # On macOS, bitcoin-qt crashes or shuts down unexpectedly during CI. Two failure
        # modes have been observed:
        # 1. ConnectionResetError: [Errno 54] Connection reset by peer — the HTTP server
        #    stops accepting connections shortly after startup, before test RPCs complete.
        #    No Qt shutdown log ("GUI: requestShutdown") is produced, ruling out the normal
        #    GUI shutdown path; the most likely cause is an external signal (SIGTERM/SIGINT)
        #    from macOS process management (launchd, jetsam, or CI runner) killing the
        #    process.
        # 2. FailedToStartError: [node 0] bitcoind exited with status -11 (SIGSEGV) — the
        #    process crashes ~193ms after window->show() is called (after
        #    "This plugin does not support propagateSizeHints()" is logged), suggesting a
        #    crash in Qt's macOS Cocoa/minimal-plugin initialization.
        if platform.system() == "Darwin":
            raise SkipTest("bitcoin-qt crashes or shuts down unexpectedly on macOS CI")

    def setup_nodes(self):
        self.extra_init = [{"use_gui": True}]
        super().setup_nodes()

    def run_test(self):
        self.log.info("Test that bitcoin-gui starts up and can be stopped via RPC")
        self.stop_node(0)


if __name__ == "__main__":
    GuiTest(__file__).main()
