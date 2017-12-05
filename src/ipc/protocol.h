// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_PROTOCOL_H
#define BITCOIN_IPC_PROTOCOL_H

#include <interfaces/init.h>

#include <memory>

namespace ipc {
//! IPC protocol interface for calling IPC methods over sockets.
//!
//! There may be different implementations of this interface for different IPC
//! protocols (e.g. Cap'n Proto, gRPC, JSON-RPC, or custom protocols).
//!
//! An implementation of this interface needs to provide an `interface::Init`
//! object that translates method calls into requests sent over a socket, and it
//! needs to implement a handler that translates requests received over a socket
//! into method calls on a provided `interface::Init` object.
class Protocol
{
public:
    virtual ~Protocol() = default;

    //! Return Init interface that forwards requests over given socket descriptor.
    //! Socket communication is handled on a background thread.
    virtual std::unique_ptr<interfaces::Init> connect(int fd) = 0;

    //! Handle requests on provided socket descriptor. Socket communication is
    //! handled on the current thread. This blocks until the client closes the socket.
    virtual void serve(int fd) = 0;
};
} // namespace ipc

#endif // BITCOIN_IPC_PROTOCOL_H
