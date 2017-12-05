// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_IPC_H
#define BITCOIN_INTERFACES_IPC_H

#include <functional>
#include <memory>
#include <typeindex>

namespace ipc {
class Context;
} // namespace ipc

namespace interfaces {
class Init;

//! Interface providing access to interprocess-communication (IPC)
//! functionality. The IPC implementation is responsible for establishing
//! connections between a controlling process and a process being controlled.
//! When a connection is established, the process being controlled returns an
//! interfaces::Init pointer to the controlling process, which the controlling
//! process can use to get access to other interfaces and functionality.
//!
//! When spawning a new process, the steps are:
//!
//! 1. The controlling process calls spawnProcess(), which calls
//!    ipc::Process::spawn(), which spawns a new process and returns a
//!    socketpair file descriptor for communicating with it. spawnProcess then
//!    calls ipc::Protocol::connect() passing the socketpair descriptor, which
//!    returns a local proxy interfaces::Init implementation calling remote
//!    interfaces::Init methods.
//! 2. The spawned process calls ipc::Process::serve(), to read command line
//!    arguments and determine whether it is a spawned process and what
//!    socketpair file descriptor it should use. It then calls
//!    ipc::Protocol::serve() to handle incoming requests from the socketpair
//!    and invoke interfaces::Init interface methods, and exit when the socket
//!    is closed.
//! 3. The controlling process calls local proxy interfaces::Init object methods
//!    to make other proxy objects calling other remote interfaces. It can also
//!    destroy the initial interfaces::Init object to close the connection and
//!    shut down the spawned process.
class Ipc
{
public:
    virtual ~Ipc() = default;

    //! Spawn a child process returning pointer to its Init interface.
    virtual std::unique_ptr<Init> spawnProcess(const char* exe_name) = 0;

    //! If this is a spawned process, block and handle requests from the parent
    //! process by forwarding them to this process's Init interface, then return
    //! true. If this is not a child process, return false.
    virtual bool serveProcess(const char* exe_name, int argc, char* argv[], int& exit_status) = 0;
<<<<<<< HEAD

    //! Add cleanup callback to remote process interface that will run when the
    //! interface is deleted.
    virtual void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) = 0;

    //! Type-safe wrapper for type-erased virtual addCleanup method.
    template<typename Interface>
    void addCleanup(Interface& interface, std::function<void()> cleanup)
    {
        addCleanup(typeid(Interface), &interface, std::move(cleanup));
    }
||||||| merged common ancestors
=======

    //! Context accessor.
    virtual ipc::Context& context() = 0;
>>>>>>> Multiprocess bitcoin
};

//! Return implementation of Ipc interface.
std::unique_ptr<Ipc> MakeIpc(int argc, char* argv[], const char* exe_name, Init& init);
} // namespace interfaces

#endif // BITCOIN_INTERFACES_IPC_H
