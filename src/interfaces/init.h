// Copyright (c) 2021 The Bitcoin Core developers
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

<<<<<<< HEAD
//! Initial interface created when a process is first started, and used to give
//! and get access to other interfaces (Node, Chain, Wallet, etc).
||||||| merged common ancestors
//! Initial interface used to get access to other interfaces.
//!
//! There are different Init interface implementation for different processes
//! (bitcoin-node, bitcoin-wallet, etc). If IPC is enabled, Init is the intial
//! interface returned over the IPC connection.
=======
//! Initial interface used to get access to other interfaces.
//!
//! There are different Init interface implementation for different processes
//! (bitcoin-node, bitcoin-wallet, etc). If IPC is enabled, Init is the initial
//! interface returned over the IPC connection.
>>>>>>> Multiprocess bitcoin
class Init
{
public:
    virtual ~Init() = default;
    virtual std::unique_ptr<Node> makeNode();
    virtual std::unique_ptr<Chain> makeChain();
    virtual std::unique_ptr<WalletClient> makeWalletClient(Chain& chain);
    virtual std::unique_ptr<Echo> makeEcho();
<<<<<<< HEAD
    virtual Ipc* ipc();
||||||| merged common ancestors
    // Note: More methods will be added here in upcoming changes as more remote
    // interfaces are supported: makeNode, makeWallet, makeWalletClient
    virtual Ipc* ipc() { return nullptr; }
=======
    virtual std::unique_ptr<Node> makeNode();
    virtual std::unique_ptr<Chain> makeChain();
    virtual std::unique_ptr<WalletClient> makeWalletClient(Chain& chain);
    virtual Ipc* ipc() { return nullptr; }
>>>>>>> Multiprocess bitcoin
};

<<<<<<< HEAD
//! Return implementation of Init interface for the node process. If the argv
//! indicates that this is a child process spawned to handle requests from a
//! parent process, this blocks and handles requests, then returns null and a
//! status code to exit with. If this returns non-null, the caller can just
//! start up normally and use the Init object to spawn and connect to other
//! processes while it is running.
||||||| merged common ancestors
//! Return implementation of Init interface for the node process.
=======
//! Return implementation of Init interface for current process.
std::unique_ptr<Init> MakeGuiInit(int argc, char* argv[]);
>>>>>>> Multiprocess bitcoin
std::unique_ptr<Init> MakeNodeInit(NodeContext& node, int argc, char* argv[], int& exit_status);
<<<<<<< HEAD

//! Return implementation of Init interface for the wallet process.
std::unique_ptr<Init> MakeWalletInit(int argc, char* argv[], int& exit_status);

//! Return implementation of Init interface for the gui process.
std::unique_ptr<Init> MakeGuiInit(int argc, char* argv[]);
||||||| merged common ancestors
=======
std::unique_ptr<Init> MakeWalletInit(int argc, char* argv[], int& exit_status);
>>>>>>> Multiprocess bitcoin
} // namespace interfaces

#endif // BITCOIN_INTERFACES_INIT_H
