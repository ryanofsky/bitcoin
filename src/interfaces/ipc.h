// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_IPC_H
#define BITCOIN_INTERFACES_IPC_H

#include <interfaces/base.h>

#include <functional>
#include <memory>

namespace ipc {
class Context;
} // namespace ipc

<<<<<<< HEAD
namespace interfaces {
class Init;
||||||| merged common ancestors
    //! Serve request if current process is a spawned subprocess. Blocks until
    //! socket for communicating with the parent process is disconnected.
    virtual bool serve(int& exit_status) = 0;
};
=======
    //! Serve request if current process is a spawned subprocess. Blocks until
    //! socket for communicating with the parent process is disconnected.
    virtual bool serve(int& exit_status) = 0;

    //! Canonicalize and connect to address, returning socket descriptor.
    virtual int connect(const fs::path& data_dir,
        const std::string& dest_exe_name,
        std::string& address,
        std::string& error) = 0;

    //! Create listening socket, bind and canonicalize address, and return socket descriptor.
    virtual int bind(const fs::path& data_dir, std::string& address, std::string& error) = 0;
};
>>>>>>> multiprocess: Add -ipcconnect and -ipcbind options

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

    //! Callback argument for spawnProcess to make a proxy object specialized
    //! for the spawned process from the initial generic interfaces::Init proxy
    //! object. Callback needs to return a reference to the proxy it creates, so
    //! spawnProcess cleanup code can delete the interfaces::Init object and
    //! close the connection when the specialized proxy is deleted.
    using MakeProxyFn = std::function<Base&(Init&)>;

<<<<<<< HEAD
    //! Spawn a process and make a proxy interface proxy object using provided
    //! callback.
    virtual void spawnProcess(const char* exe_name, const MakeProxyFn& make_proxy) = 0;
||||||| merged common ancestors
    //! Handle requests on provided socket descriptor. Socket communication is
    //! handled on the current thread. This blocks until the client closes the socket.
    //!
    //! @note: If this method is called, it needs be called before connect()
    //! because for ease of implementation it's inflexible and always runs the
    //! event loop in the foreground thread. It can share its event loop with
    //! connect() but can't share an event loop that was created by connect().
    //! This isn't really a problem because serve() is only called by spawned
    //! child processes that call it immediately to communicate back with parent
    //! processes.
    virtual void serve(int fd) = 0;
};
=======
    //! Listen for connections on provided socket descriptor, accept them, and
    //! handle requests on accepted connections. This method doesn't block, and
    //! performs I/O on a background thread.
    virtual void listen(int listen_fd) = 0;

    //! Handle requests on provided socket descriptor. Socket communication is
    //! handled on the current thread. This blocks until the client closes the socket.
    //!
    //! @note: If this method is called, it needs be called before connect() or
    //! listen() methods, because for ease of implementation it's inflexible and
    //! always runs the event loop in the foreground thread. It can share its
    //! event loop with the other methods but can't share an event loop that was
    //! created by them. This isn't really a problem because serve() is only
    //! called by spawned child processes that call it immediately to
    //! communicate back with parent processes.
    virtual void serve(int fd) = 0;
};
>>>>>>> multiprocess: Add -ipcconnect and -ipcbind options

    //! Serve requests if current process is a spawned subprocess. Blocks until
    //! parent process is disconnected.
    virtual bool serveProcess(const char* exe_name, int argc, char* argv[], int& exit_status) = 0;

    //! Context accessor.
    virtual ipc::Context& context() = 0;
};

//! Return implementation of Ipc interface.
std::unique_ptr<Ipc> MakeIpc(int argc, char* argv[], const char* exe_name, Init& init);
} // namespace interfaces

#endif // BITCOIN_INTERFACES_IPC_H
