// Copyright (c) 2020 The Bitcoin Core developers
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
