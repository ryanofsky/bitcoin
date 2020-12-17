// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_PROCESS_H
#define BITCOIN_IPC_PROCESS_H

#include <memory>
#include <string>

namespace ipc {
class Protocol;

//! IPC process interface for spawning bitcoin processes and serving requests
//! in processes that have been spawned.
//!
//! There will different implementations of this interface depending on the
//! platform (e.g. unix, windows).
class Process
{
public:
    virtual ~Process() = default;

    //! Spawn process and return socket file descriptor for communicating with
    //! it.
    virtual int spawn(const std::string& new_exe_name, int& pid) = 0;

    //! Wait for spawned process to exit and return exit code.
    virtual int wait(int pid) = 0;

    //! Serve requests if current process is a spawned subprocess. Blocks until
    //! socket for communicating with the parent process is disconnected.
    virtual bool serve(int& exit_status) = 0;
};

//! Constructor for Process interface. Implementation will vary depending on
//! the platform (unix or windows).
std::unique_ptr<Process> MakeProcess(int argc, char* argv[], const char* exe_name, Protocol& protocol);
} // namespace ipc

#endif // BITCOIN_IPC_PROCESS_H
