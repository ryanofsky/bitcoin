// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_INIT_H
#define BITCOIN_INTERFACES_INIT_H

#include <memory>

struct NodeContext;

namespace interfaces {
class Chain;
class Echo;
class Ipc;
class Node;
class WalletClient;

//! Initial interface used to get access to other interfaces.
//!
//! There are different Init interface implementation for different processes
//! (bitcoin-node, bitcoin-wallet, etc). If IPC is enabled, Init is the initial
//! interface returned over the IPC connection.
class Init
{
public:
    virtual ~Init() = default;
    virtual std::unique_ptr<Echo> makeEcho();
    virtual std::unique_ptr<Node> makeNode();
    virtual std::unique_ptr<Chain> makeChain();
    virtual std::unique_ptr<WalletClient> makeWalletClient(Chain& chain);
    virtual Ipc* ipc() { return nullptr; }
};

//! Return implementation of Init interface for current process.
std::unique_ptr<Init> MakeGuiInit(int argc, char* argv[]);
std::unique_ptr<Init> MakeNodeInit(NodeContext& node, int argc, char* argv[], int& exit_status);
std::unique_ptr<Init> MakeWalletInit(int argc, char* argv[], int& exit_status);
} // namespace interfaces

#endif // BITCOIN_INTERFACES_INIT_H
