// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_INIT_H
#define BITCOIN_INTERFACES_INIT_H

#include <fs.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct NodeContext;

namespace interfaces {
class Base;
class IpcProcess;
class IpcProtocol;

//! Init interface providing access to other interfaces. The Init interface is
//! the interface first exposed when a process spawns or connects to another
//! process.
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
//!    and determine whether it is a spawned process and what socketpair file
//!    descriptor it should use. It calls IpcProtocol::serve() to handle
//!    incoming requests from the socketpair and call Init interface methods, or
//!    destroy the Init interface and shut down the process.
//!
//! When connecting to an existing process, the steps are similar to spawning a
//! new process, except a domain socket is created instead of a socketpair, and
//! destroying an Init interface doesn't end the process, since there can be
//! multiple connections.
class Init
{
public:
    virtual ~Init() = default;
};

//! Specialization of the Init interface for the local process. Container for
//! IpcProcess and IpcProtocol objects and current process information.
class LocalInit : public Init
{
public:
    LocalInit(const char* exe_name, const char* log_suffix);
    ~LocalInit() override;
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
} // namespace interfaces

#endif // BITCOIN_INTERFACES_INIT_H
