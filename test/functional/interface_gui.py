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

    def setup_nodes(self):
        self.extra_init = [{"use_gui": True}]
        super().setup_nodes()

    def run_test(self):
        self.log.info("Test that bitcoin-gui starts up and can be stopped via RPC")
        self.stop_node(0)


if __name__ == "__main__":
    GuiTest(__file__).main()
