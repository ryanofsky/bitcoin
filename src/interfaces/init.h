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

<<<<<<< HEAD
//! Return implementation of Init interface for current process.
std::unique_ptr<Init> MakeGuiInit(int argc, char* argv[]);
std::unique_ptr<Init> MakeNodeInit(NodeContext& node, int argc, char* argv[], int& exit_status);
std::unique_ptr<Init> MakeWalletInit(int argc, char* argv[], int& exit_status);
||||||| merged common ancestors
//! Create interface pointers used by current process.
std::unique_ptr<LocalInit> MakeInit(int argc, char* argv[]);

//! Callback provided to SpawnProcess to make a new client interface proxy
//! object from an existing client Init interface proxy object. Callback needs
//! to return a reference to the client it creates, so SpawnProcess can add
//! close hooks and shut down the spawned process when the client is destroyed.
using MakeClientFn = std::function<Base&(Init&)>;

//! Helper to spawn a process and make a client interface proxy object using
//! provided callback.
void SpawnProcess(IpcProcess& process,
    IpcProtocol& protocol,
    const std::string& new_exe_name,
    const MakeClientFn& make_client);
=======
//! Create interface pointers used by current process.
std::unique_ptr<LocalInit> MakeInit(int argc, char* argv[]);

//! Callback provided to SpawnProcess and ConnectAddress to make a new client
//! interface proxy object from an existing client Init interface proxy object.
//! Callback needs to return a reference to the client it creates, so
//! SpawnProcess and ConnectAddress can add close hooks and shut down the
//! connection when the client is destroyed.
using MakeClientFn = std::function<Base&(Init&)>;

//! Helper to spawn a process and make a client interface proxy object using
//! provided callback.
void SpawnProcess(IpcProcess& process,
    IpcProtocol& protocol,
    const std::string& new_exe_name,
    const MakeClientFn& make_client);

//! Helper to connect to a socket address and make a client interface proxy
//! object using provided callback.
bool ConnectAddress(IpcProcess& process, IpcProtocol& protocol, const fs::path& data_dir, std::string& address, const MakeClientFn& make_client);

//! Connect to chain in existing bitcoin-node process.
std::unique_ptr<Chain> ConnectChain(LocalInit& local_init, const fs::path& data_dir, std::string& address);
>>>>>>> multiprocess: Add -ipcconnect and -ipcbind options
} // namespace interfaces

#endif // BITCOIN_INTERFACES_INIT_H
