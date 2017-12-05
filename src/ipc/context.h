<<<<<<< HEAD
// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CONTEXT_H
#define BITCOIN_IPC_CONTEXT_H

namespace ipc {
//! Context struct used to give IPC protocol implementations or implementation
//! hooks access to application state, in case they need to run extra code that
//! that isn't needed within a single procoess, like code copying global state
//! from an existing process to a new process when it's initialized, or code
//! dealing with shared objects that are created or destroyed remotely.
struct Context
{
};
} // namespace ipc

#endif // BITCOIN_IPC_CONTEXT_H
||||||| merged common ancestors
=======
// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CONTEXT_H
#define BITCOIN_IPC_CONTEXT_H

#include <functional>

namespace ipc {
//! Context to give IPC protocol hooks access to application state.
struct Context
{
    //! Callback to initialize spawned process after receiving ArgsManager
    //! configuration from parent.
    std::function<void()> init_process;
};
} // namespace ipc

#endif // BITCOIN_IPC_CONTEXT_H
>>>>>>> Add ipc::Context and ipc::capnp::Context structs
