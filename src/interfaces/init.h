// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_INIT_H
#define BITCOIN_INTERFACES_INIT_H

<<<<<<< HEAD
||||||| merged common ancestors
#include <fs.h>
#include <functional>
=======
#include <util/ref.h>

#include <fs.h>
#include <functional>
>>>>>>> Multiprocess bitcoin
#include <memory>

struct NodeContext;

namespace interfaces {
<<<<<<< HEAD
||||||| merged common ancestors
class Base;
=======
class Base;
class Chain;
>>>>>>> Multiprocess bitcoin
class Echo;
<<<<<<< HEAD
class Ipc;
||||||| merged common ancestors
class IpcProcess;
class IpcProtocol;
=======
class IpcProcess;
class IpcProtocol;
class Node;
class WalletClient;
struct NodeClientParam;
struct NodeServerParam;
>>>>>>> Multiprocess bitcoin

//! Initial interface used to get access to other interfaces.
//!
<<<<<<< HEAD
//! There are different Init interface implementation for different processes
//! (bitcoin-node, bitcoin-wallet, etc). If IPC is enabled, Init is the intial
//! interface returned over the IPC connection.
||||||| merged common ancestors
//! The interfaces::Init interface is similar to other cross-process C++
//! interfaces like interfaces::Node, interfaces::Wallet, interfaces::Chain and
//! interfaces::ChainClient, providing virtual methods that can be called from
//! other processes. What makes it special is that unlike other interfaces which
//! are not implemented by every process—interfaces::Node is only implemented by
//! the node process and interfaces::Wallet is only implemented by the wallet
//! process—interfaces::Init is implemented by every process that supports being
//! spawned, and it is the initial interface returned by the
//! IpcProtocol::connect(fd) method, allowing the parent process to control the
//! child process after the connection is established. The interfaces::Init
//! interface has methods that allow the parent process to get access to every
//! interface supported by the child process, and when the parent process frees
//! the interfaces::Init object, the child process shuts down.
//!
//! When spawning a new process, the steps are:
//!
//! 1. Client process calls IpcProcess::spawn(), which spawns a new process and
//!    returns a socketpair file descriptor for communicating with it.
//! 2. Client process calls IpcProtocol::connect() passing the socketpair
//!    descriptor, which returns a client Init proxy object calling remote Init
//!    interface methods.
//! 3. Client process calls client Init proxy object methods to make new client
//!    proxy objects calling other remote interfaces. It can also destroy the
//!    Init object to shut down the spawned process.
//! 4. Spawned process calls IpcProcess::serve(), to read command line arguments
//!    and determine that it is a spawned process and what socketpair file
//!    descriptor it should use. The spawned process then calls
//!    IpcProtocol::serve() to handle incoming requests from the socketpair and
//!    invoke Init interface methods and eventually shut down and destroy the
//!    Init interface when the connection is closed.
//!
//! When connecting to an existing process, the steps are similar to spawning a
//! new process, except a socket is created instead of a socketpair, and
//! destroying an Init interface doesn't end the process, since there can be
//! multiple connections.
=======
//! The interfaces::Init interface is similar to other cross-process C++
//! interfaces like interfaces::Node, interfaces::Wallet, interfaces::Chain and
//! interfaces::WalletClient, providing virtual methods that can be called from
//! other processes. What makes it special is that unlike other interfaces which
//! are not implemented by every process—interfaces::Node is only implemented by
//! the node process and interfaces::Wallet is only implemented by the wallet
//! process—interfaces::Init is implemented by every process that supports being
//! spawned, and it is the initial interface returned by the
//! IpcProtocol::connect(fd) method, allowing the parent process to control the
//! child process after the connection is established. The interfaces::Init
//! interface has methods that allow the parent process to get access to every
//! interface supported by the child process, and when the parent process frees
//! the interfaces::Init object, the child process shuts down.
//!
//! When spawning a new process, the steps are:
//!
//! 1. Client process calls IpcProcess::spawn(), which spawns a new process and
//!    returns a socketpair file descriptor for communicating with it.
//! 2. Client process calls IpcProtocol::connect() passing the socketpair
//!    descriptor, which returns a client Init proxy object calling remote Init
//!    interface methods.
//! 3. Client process calls client Init proxy object methods to make new client
//!    proxy objects calling other remote interfaces. It can also destroy the
//!    Init object to shut down the spawned process.
//! 4. Spawned process calls IpcProcess::serve(), to read command line arguments
//!    and determine that it is a spawned process and what socketpair file
//!    descriptor it should use. The spawned process then calls
//!    IpcProtocol::serve() to handle incoming requests from the socketpair and
//!    invoke Init interface methods and eventually shut down and destroy the
//!    Init interface when the connection is closed.
//!
//! When connecting to an existing process, the steps are similar to spawning a
//! new process, except a socket is created instead of a socketpair, and
//! destroying an Init interface doesn't end the process, since there can be
//! multiple connections.
>>>>>>> Multiprocess bitcoin
class Init
{
public:
    virtual ~Init() = default;
<<<<<<< HEAD
    virtual std::unique_ptr<Echo> makeEcho();
    // Note: More methods will be added here in upcoming changes as more remote
    // interfaces are supported: makeNode, makeWallet, makeWalletClient
    virtual Ipc* ipc() { return nullptr; }
||||||| merged common ancestors
    virtual std::unique_ptr<Echo> makeEcho() = 0;
=======
    virtual std::unique_ptr<Echo> makeEcho() = 0;
    virtual std::unique_ptr<Node> makeNode() = 0;
    virtual std::unique_ptr<Chain> makeChain() = 0;
    virtual std::unique_ptr<WalletClient> makeWalletClient(Chain& chain) = 0;
>>>>>>> Multiprocess bitcoin
};

<<<<<<< HEAD
//! Return implementation of Init interface for the node process.
std::unique_ptr<Init> MakeNodeInit(NodeContext& node, int argc, char* argv[], int& exit_status);
||||||| merged common ancestors
//! Specialization of the Init interface for the local process. Container for
//! IpcProcess and IpcProtocol objects and current process information.
class LocalInit : public Init
{
public:
    LocalInit(const char* exe_name, const char* log_suffix);
    ~LocalInit() override;
    std::unique_ptr<Echo> makeEcho() override;
    //! Make echo implementation for `echoipc` test RPC. Spawn new process if
    //! supported.
    virtual std::unique_ptr<Echo> makeEchoIpc();
    //! Return node context if current process is a node process (not available
    //! in wallet or gui-only processes).
    virtual NodeContext& node();
    const char* m_exe_name;
    const char* m_log_suffix;
    std::unique_ptr<IpcProtocol> m_protocol;
    std::unique_ptr<IpcProcess> m_process;
};

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
//! Specialization of the Init interface for the local process. Container for
//! IpcProcess and IpcProtocol objects and current process information.
class LocalInit : public Init
{
public:
    LocalInit(const char* exe_name, const char* log_suffix);
    ~LocalInit() override;
    std::unique_ptr<Echo> makeEcho() override;
    std::unique_ptr<Node> makeNode() override;
    std::unique_ptr<Chain> makeChain() override;
    std::unique_ptr<WalletClient> makeWalletClient(Chain& chain) override;
    //! Do extra initialization needed to initialize the second gui/node/wallet
    //! process when code is running in a new process, instead of the process
    //! that called it.
    //!
    //! Because gui/node/wallet processes all have slightly different init
    //! sequences (this should be cleaned up), the implementation of this method
    //! varies between the three processes, but in all cases brings them up to
    //! the point where global ECC crypto, config, and chain variables are
    //! initialized and logging is started.
    //!
    //! - For bitcoind and bitcoin-qt, this is unimplemented and isn't called.
    //!
    //! - For bitcoin-gui, this is called after the normal ECC and config parsing
    //!   code before spawning/connecting and just starts logging.
    //!
    //! - For bitcoin-node, this is not called by default, but if bitcoin-node
    //!   is spawned internally by the gui, this is called from
    //!   Node.baseInitialize after receiving GUI GlobalArgs catch up the
    //!   spawned process to current point in GUI initialization sequence
    //!   instead of its own.
    //!
    //! - For bitcoin-wallet, this is not called by default, but if
    //!   bitcoin-wallet is spawned internally by the node, it's called from
    //!   Init.makeWalletClient after receiving node GlobalArgs to prepare
    //!   spawned process for the node's initialization sequence of loading
    //!   wallet files and registering RPCs.
    virtual void initProcess() {}
    //! Make echo implementation for `echoipc` test RPC. Spawn new process if
    //! supported.
    virtual std::unique_ptr<Echo> makeEchoIpc();
    virtual void makeNodeServer(NodeServerParam&) {}
    virtual void makeNodeClient(NodeClientParam&) {}
    //! Return node context if current process is a node process (not available
    //! in wallet or gui-only processes).
    virtual NodeContext& node();
    const char* m_exe_name;
    const char* m_log_suffix;
    std::unique_ptr<IpcProtocol> m_protocol;
    std::unique_ptr<IpcProcess> m_process;
};

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
>>>>>>> Multiprocess bitcoin
} // namespace interfaces

#endif // BITCOIN_INTERFACES_INIT_H
