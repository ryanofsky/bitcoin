// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_INIT_H
#define BITCOIN_INTERFACES_INIT_H

#include <memory>

struct NodeContext;

namespace interfaces {
class Ipc;

//! Initial interface used to get access to other interfaces.
//!
//! There are different Init interface implementation for different processes
//! (bitcoin-node, bitcoin-wallet, etc). If IPC is enabled, Init is the intial
//! interface returned over the IPC connection.
class Init
{
public:
    virtual ~Init() = default;
    // Note: More methods will be added here in upcoming changes as more remote
    // interfaces are supported: makeNode, makeWallet, makeWalletClient
    virtual Ipc* ipc() { return nullptr; }
};

//! Return implementation of Init interface for the node process.
std::unique_ptr<Init> MakeNodeInit(NodeContext& node, int argc, char* argv[], int& exit_status);
} // namespace interfaces

#endif // BITCOIN_INTERFACES_INIT_H
