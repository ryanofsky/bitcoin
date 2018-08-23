// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_IPC_H
#define BITCOIN_INTERFACES_IPC_H

#include <fs.h>
#include <interfaces/init.h>

#include <memory>

namespace interfaces {
//! IPC process interface for spawning bitcoin processes and serving requests
//! in processes that have been spawned.
//!
//! There will different implementations of this interface depending on the
//! platform (e.g. unix, windows).
class IpcProcess
{
public:
    virtual ~IpcProcess() = default;

    //! Spawn process and return socket file descriptor for communicating with
    //! it.
    virtual int spawn(const std::string& new_exe_name, int& pid) = 0;

    //! Wait for spawned process to exit and return exit code.
    virtual int wait(int pid) = 0;

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

//! IPC protocol interface for calling IPC methods over sockets.
//!
//! There may be different implementations of this interface for different IPC
//! protocols (e.g. Cap'n Proto, gRPC, JSON-RPC, or custom protocols).
//!
//! An implementation of this interface needs to provide an `interface::Init`
//! object that translates method calls into requests sent over a socket, and it
//! needs to implement a handler that translates requests received over a socket
//! into method calls on a provided `interface::Init` object.
class IpcProtocol
{
public:
    virtual ~IpcProtocol() = default;

    //! Return Init interface that forwards requests over given socket descriptor.
    //! Socket communication is handled on a background thread.
    //!
    //! @note It could be potentially useful in the future to add
    //! std::function<void()> on_disconnect callback argument here. But there
    //! isn't an immediate need, because the protocol implementation can clean
    //! up its own state (calling ProxyServer destructors, etc) on disconnect,
    //! and any client calls will just throw IpcException errors after a
    //! disconnect.
    virtual std::unique_ptr<Init> connect(int fd) = 0;

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

//! Exception class thrown when a call to remote method fails due to an IPC
//! error, like a socket getting disconnected.
class IpcException : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

//! Constructor for IpcProcess interface. Implementation will vary depending on
//! the platform (unix or windows).
std::unique_ptr<IpcProcess> MakeIpcProcess(int argc, char* argv[], const char* exe_name, IpcProtocol& protocol);
} // namespace interfaces

#endif // BITCOIN_INTERFACES_IPC_H
