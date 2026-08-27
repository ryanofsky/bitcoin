#!/usr/bin/env python3
# Copyright (c) The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test that bitcoin-gui starts up and can be stopped via RPC."""

import pathlib
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
        if platform.system() == "Windows":
            bin_dir = pathlib.Path(self.binary_paths.bitcoin_bin).parent
            # Dynamic Qt builds need platforms/qminimal.dll to run headless.
            # Static Qt builds (cross-compiled) have the minimal plugin compiled
            # in and work without it. Detect dynamic Qt by the presence of Qt
            # DLLs alongside the binary (static builds don't have them).
            has_dynamic_qt = any(bin_dir.glob("Qt6*.dll")) or any(bin_dir.glob("Qt5*.dll"))
            if has_dynamic_qt and not (bin_dir / "platforms" / "qminimal.dll").exists():
                raise SkipTest("platforms/qminimal.dll not deployed; "
                               "headless GUI testing requires the minimal Qt platform plugin")

    def setup_nodes(self):
        self.extra_init = [{"use_gui": True}]
        super().setup_nodes()

    def run_test(self):
        self.log.info("Test that bitcoin-gui starts up and can be stopped via RPC")
        self.stop_node(0)


if __name__ == "__main__":
    GuiTest(__file__).main()
