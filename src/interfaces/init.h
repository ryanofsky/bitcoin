// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_INIT_H
#define BITCOIN_INTERFACES_INIT_H

#include <memory>

struct NodeContext;

namespace interfaces {
class Chain;
class Ipc;
class Node;
class WalletClient;

//! Initial interface created when a process is first started, and used to give
//! and get access to other interfaces (Node, Chain, Wallet, etc).
class Init
{
public:
    virtual ~Init() = default;
    virtual std::unique_ptr<Node> makeNode();
    virtual std::unique_ptr<Chain> makeChain();
    virtual std::unique_ptr<WalletClient> makeWalletClient(Chain& chain);
    virtual Ipc* ipc();
};
} // namespace interfaces

#endif // BITCOIN_INTERFACES_INIT_H
