// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <ipc/capnp/protocol.h>
#include <ipc/process.h>
#include <ipc/protocol.h>
#include <logging.h>
#include <tinyformat.h>
#include <util/memory.h>
#include <util/system.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

namespace ipc {
namespace {

class IpcImpl : public interfaces::Ipc
{
public:
    IpcImpl(int argc, char* argv[], const char* exe_name, interfaces::Init& init, bool can_connect, bool can_listen)
        : m_protocol(ipc::capnp::MakeCapnpProtocol(exe_name, init)),
          m_process(ipc::MakeProcess(argc, argv, exe_name, *m_protocol)), m_can_connect(can_connect),
          m_can_listen(can_listen)
    {
    }
    std::unique_ptr<interfaces::Init> spawnProcess(const char* exe_name) override
    {
        int pid;
        int fd = m_process->spawn(exe_name, pid);
        LogPrint(::BCLog::IPC, "Process %s pid %i launched\n", exe_name, pid);
        auto init = m_protocol->connect(fd);
        Ipc::addCleanup(*init, [this, exe_name, pid] {
            int status = m_process->wait(pid);
            LogPrint(::BCLog::IPC, "Process %s pid %i exited with status %i\n", exe_name, pid, status);
        });
        return init;
    }
    bool serveProcess(const char* exe_name, int argc, char* argv[], int& exit_status) override
    {
        if (m_process->serve(exit_status)) {
            LogPrint(::BCLog::IPC, "Process %s exiting with status %i\n", exe_name, exit_status);
            return true;
        }
        return false;
    }
    bool canConnect() override { return m_can_connect; }
    std::unique_ptr<interfaces::Init> connectAddress(std::string& address) override
    {
        if (address.empty() || address == "0") return nullptr;
        int fd = -1;
        std::string error;
        if (address == "auto") {
            // failure to connect with "auto" isn't an error. Caller can spawn a child process or just work offline.
            address = "unix";
            fd = m_process->connect(GetDataDir(), "bitcoin-node", address, error);
            if (fd < 0) return nullptr;
        } else {
            fd = m_process->connect(GetDataDir(), "bitcoin-node", address, error);
        }
        if (fd < 0) {
            throw std::runtime_error(
                strprintf("Could not connect to bitcoin-node IPC address '%s'. %s", address, error));
        }
        return m_protocol->connect(fd);
    }
    bool canListen() override { return m_can_listen; }
    bool listenAddress(std::string& address, std::string& error) override
    {
        int fd = m_process->bind(GetDataDir(), address, error);
        if (fd < 0) return false;
        m_protocol->listen(fd);
        return true;
    }
    void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) override
    {
        m_protocol->addCleanup(type, iface, std::move(cleanup));
    }
    Context& context() override { return m_protocol->context(); }
    std::unique_ptr<Protocol> m_protocol;
    std::unique_ptr<Process> m_process;
    bool m_can_connect;
    bool m_can_listen;
};
} // namespace
} // namespace ipc

namespace interfaces {
std::unique_ptr<Ipc> MakeIpc(
    int argc, char* argv[], const char* exe_name, Init& init, bool can_connect, bool can_listen)
{
    return MakeUnique<ipc::IpcImpl>(argc, argv, exe_name, init, can_connect, can_listen);
}
} // namespace interfaces
