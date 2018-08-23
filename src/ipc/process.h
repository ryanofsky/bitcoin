// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_PROCESS_H
#define BITCOIN_IPC_PROCESS_H

#include <fs.h>

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

    //! Wait for spawned process to exit and return its exit code.
    virtual int wait(int pid) = 0;

    //! Serve requests if current process is a spawned subprocess. Blocks until
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

//! Constructor for Process interface. Implementation will vary depending on
//! the platform (unix or windows).
std::unique_ptr<Process> MakeProcess(int argc, char* argv[], const char* exe_name, Protocol& protocol);
} // namespace ipc

#endif // BITCOIN_IPC_PROCESS_H
