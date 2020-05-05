#!/usr/bin/env python3
# Copyright (c) 2014-2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the -alertnotify, -blocknotify and -walletnotify options."""
import os

from test_framework.address import ADDRESS_BCRT1_UNSPENDABLE, keyhash_to_p2pkh
from test_framework.test_framework import BitcoinTestFramework
from test_framework.util import assert_equal, wait_until, connect_nodes_bi, connect_nodes, disconnect_nodes, hex_str_to_bytes


class NotificationsTest(BitcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2
        self.setup_clean_chain = True

    def setup_network(self):
        self.alertnotify_dir = os.path.join(self.options.tmpdir, "alertnotify")
        self.blocknotify_dir = os.path.join(self.options.tmpdir, "blocknotify")
        self.walletnotify_dir = os.path.join(self.options.tmpdir, "walletnotify")
        os.mkdir(self.alertnotify_dir)
        os.mkdir(self.blocknotify_dir)
        os.mkdir(self.walletnotify_dir)

        # -alertnotify and -blocknotify on node0, walletnotify on node1
        self.extra_args = [[
                            "-alertnotify=echo > {}".format(os.path.join(self.alertnotify_dir, '%s')),
                            "-blocknotify=echo > {}".format(os.path.join(self.blocknotify_dir, '%s'))],
                           ["-blockversion=211",
                            "-rescan",
                            "-walletnotify=echo > {}".format(os.path.join(self.walletnotify_dir, '%s'))]]
        super().setup_network()

    def run_test(self):
        self.log.info("test -blocknotify")
        block_count = 10
        blocks = self.nodes[1].generatetoaddress(block_count, self.nodes[1].getnewaddress() if self.is_wallet_compiled() else ADDRESS_BCRT1_UNSPENDABLE)

        # wait at most 10 seconds for expected number of files before reading the content
        wait_until(lambda: len(os.listdir(self.blocknotify_dir)) == block_count, timeout=10)

        # directory content should equal the generated blocks hashes
        assert_equal(sorted(blocks), sorted(os.listdir(self.blocknotify_dir)))

        if self.is_wallet_compiled():
            self.log.info("test -walletnotify")
            # wait at most 10 seconds for expected number of files before reading the content
            wait_until(lambda: len(os.listdir(self.walletnotify_dir)) == block_count, timeout=10)

            # directory content should equal the generated transaction hashes
            txids_rpc = list(map(lambda t: t['txid'], self.nodes[1].listtransactions("*", block_count)))
            assert_equal(sorted(txids_rpc), sorted(os.listdir(self.walletnotify_dir)))
            self.stop_node(1)
            for tx_file in os.listdir(self.walletnotify_dir):
                os.remove(os.path.join(self.walletnotify_dir, tx_file))

            self.log.info("test -walletnotify after rescan")
            # restart node to rescan to force wallet notifications
            self.start_node(1)
            connect_nodes_bi(self.nodes, 0, 1)

            wait_until(lambda: len(os.listdir(self.walletnotify_dir)) == block_count, timeout=10)

            # directory content should equal the generated transaction hashes
            txids_rpc = list(map(lambda t: t['txid'], self.nodes[1].listtransactions("*", block_count)))
            assert_equal(sorted(txids_rpc), sorted(os.listdir(self.walletnotify_dir)))
            for tx_file in os.listdir(self.walletnotify_dir):
                os.remove(os.path.join(self.walletnotify_dir, tx_file))

            # Give node 0 same node key as node 1, generate spends from node 0,
            # and make sure notifications are triggered for node 1
            self.nodes[0].sethdseed(True, self.nodes[1].dumpprivkey(keyhash_to_p2pkh(hex_str_to_bytes(self.nodes[1].getwalletinfo()['hdseedid'])[::-1])))
            self.nodes[0].rescanblockchain()
            self.nodes[0].generatetoaddress(100, ADDRESS_BCRT1_UNSPENDABLE)

            # Generate two transactions on node 0, sync mempools, and check for
            # notifications on node 1.
            tx1 = self.nodes[0].sendtoaddress(address=ADDRESS_BCRT1_UNSPENDABLE, amount=1, replaceable=True)
            self.sync_mempools()
            assert_equal(tx1 in self.nodes[0].getrawmempool(), True)
            assert_equal(tx1 in self.nodes[1].getrawmempool(), True)
            self.expect_wallet_notify([tx1])

            # Generate bump transaction, sync mempools, and check for bump1
            # notification. In the future, per
            # https://github.com/bitcoin/bitcoin/pull/9371, it might be better
            # to notify tx1 as well as bump1.
            bump1 = self.nodes[0].bumpfee(tx1)["txid"]
            self.sync_mempools()
            assert_equal(bump1 in self.nodes[0].getrawmempool(), True)
            assert_equal(bump1 in self.nodes[1].getrawmempool(), True)
            self.expect_wallet_notify([bump1])

            # Add bump1 transaction to new block, checking for a notification
            # and correct number of confirmations
            self.nodes[0].generatetoaddress(1, ADDRESS_BCRT1_UNSPENDABLE)
            self.sync_blocks()
            self.expect_wallet_notify([bump1])
            assert_equal(self.nodes[1].gettransaction(bump1)["confirmations"], 1)

            # Generate a second transaction to be bumped
            tx2 = self.nodes[0].sendtoaddress(address=ADDRESS_BCRT1_UNSPENDABLE, amount=1, replaceable=True)
            self.sync_mempools()
            assert_equal(tx2 in self.nodes[0].getrawmempool(), True)
            assert_equal(tx2 in self.nodes[1].getrawmempool(), True)
            self.expect_wallet_notify([tx2])

            # Bump tx2 and add it to a block on node 0 while disconnected, then
            # reconnect and check for two notifications on node 1 about newly
            # confirmed bump2 and conflicted tx2.
            disconnect_nodes(self.nodes[0], 1)
            bump2 = self.nodes[0].bumpfee(tx2)["txid"]
            self.nodes[0].generatetoaddress(1, ADDRESS_BCRT1_UNSPENDABLE)
            assert_equal(self.nodes[0].gettransaction(bump2)["confirmations"], 1)
            assert_equal(tx2 in self.nodes[1].getrawmempool(), True)
            connect_nodes(self.nodes[0], 1)
            self.sync_blocks()
            self.expect_wallet_notify([bump2, tx2])
            assert_equal(self.nodes[1].gettransaction(bump2)["confirmations"], 1)


        # TODO: add test for `-alertnotify` large fork notifications

    def expect_wallet_notify(self, tx_ids):
        wait_until(lambda: len(os.listdir(self.walletnotify_dir)) >= len(tx_ids), timeout=10)
        assert_equal(sorted(tx_ids), sorted(os.listdir(self.walletnotify_dir)))
        for tx_file in os.listdir(self.walletnotify_dir):
            os.remove(os.path.join(self.walletnotify_dir, tx_file))


if __name__ == '__main__':
    NotificationsTest().main()
