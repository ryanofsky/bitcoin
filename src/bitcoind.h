// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_BITCOIND_H
#define BITCOIN_BITCOIND_H

namespace node {
struct NodeContext;
} // namespace node

bool ParseArgs(node::NodeContext& node, int argc, char* argv[]);
bool AppInit(node::NodeContext& node);
int NodeMain(node::NodeContext& node, int argc, char* argv[]);

#endif // BITCOIN_BITCOIND_H
