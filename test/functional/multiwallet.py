#!/usr/bin/env python3
# Copyright (c) 2017 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test multiwallet.

Verify that a bitcoind node can load multiple wallet files
"""
import os
import shutil

from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, assert_raises_rpc_error

class MultiWalletTest(BitcoinTestFramework):
    def set_test_params(self):
        self.setup_clean_chain = True
        self.num_nodes = 1

    def run_test(self):
        # helper to construct node0 datadir paths
        data_path = lambda *p: os.path.join(self.options.tmpdir, 'node0', 'regtest', *p)

        # check wallet.dat is created
        self.stop_node(0)
        assert_equal(os.path.isfile(data_path("wallet.dat")), True)

        # rename wallet.dat to make sure bitcoin can open two wallets in the
        # same directory (for backwards compatibility)
        os.rename(data_path("wallet.dat"), data_path("other_wallet.dat"))

        # restart node with a mix of wallet arguments
        os.mkdir(data_path("w6"))
        os.symlink("w6", data_path("w6_symlink"))
        wallet_names = ['w1', 'w2', 'w3', 'sub/w4', os.path.join(self.options.tmpdir, "extern/w5"), "w6_symlink", "", "other_wallet.dat"]
        extra_args = ['-wallet={}'.format(n) for n in wallet_names]
        self.start_node(0, extra_args)
        assert_equal(set(self.nodes[0].listwallets()), set(wallet_names))

        # check that all requested wallets were created
        self.stop_node(0)
        for wallet_name in wallet_names:
            if os.path.isdir(data_path(wallet_name)):
                assert_equal(os.path.isfile(data_path(wallet_name, "wallet.dat")), True)
            else:
                assert_equal(os.path.isfile(data_path(wallet_name)), True)

        # should not initialize if there are duplicate wallets
        self.assert_start_raises_init_error(0, ['-wallet=w1', '-wallet=w1'], 'Error loading wallet w1. Duplicate -wallet filename specified.')

        # should not initialize if wallet directory can't be created
        self.assert_start_raises_init_error(0, ['-wallet=wallet.dat/bad'], 'create_directories: Not a directory')

        # should not initialize if two wallets in the same environment are copies of each other
        shutil.copyfile(data_path('wallet.dat'), data_path('wallet_copy.dat'))
        self.assert_start_raises_init_error(0, ['-wallet=wallet.dat', '-wallet=wallet_copy.dat'], 'duplicates fileid')

        # should not initialize if wallet file is a symlink
        os.symlink("wallet.dat", data_path("wallet_symlink.dat"))
        self.assert_start_raises_init_error(0, ['-wallet=wallet_symlink.dat'], 'wallet path cannot be a symlink to a file')

        self.start_node(0, extra_args)

        wallets = [self.nodes[0].get_wallet_rpc(w) for w in wallet_names]
        wallet_bad = self.nodes[0].get_wallet_rpc("bad")

        # check wallet names and balances
        wallets[0].generate(1)
        for wallet_name, wallet in zip(wallet_names, wallets):
            info = wallet.getwalletinfo()
            assert_equal(info['immature_balance'], 50 if wallet is wallets[0] else 0)
            assert_equal(info['walletname'], wallet_name)

        # accessing invalid wallet fails
        assert_raises_rpc_error(-18, "Requested wallet does not exist or is not loaded", wallet_bad.getwalletinfo)

        # accessing wallet RPC without using wallet endpoint fails
        assert_raises_rpc_error(-19, "Wallet file not specified", self.nodes[0].getwalletinfo)

        w1, w2, w3, *_ = wallets
        w1.generate(101)
        assert_equal(w1.getbalance(), 100)
        assert_equal(w2.getbalance(), 0)
        assert_equal(w3.getbalance(), 0)

        w1.sendtoaddress(w2.getnewaddress(), 1)
        w1.sendtoaddress(w3.getnewaddress(), 2)
        w1.generate(1)
        assert_equal(w2.getbalance(), 1)
        assert_equal(w3.getbalance(), 2)

        batch = w1.batch([w1.getblockchaininfo.get_request(), w1.getwalletinfo.get_request()])
        assert_equal(batch[0]["result"]["chain"], "regtest")
        assert_equal(batch[1]["result"]["walletname"], "w1")

if __name__ == '__main__':
    MultiWalletTest().main()
