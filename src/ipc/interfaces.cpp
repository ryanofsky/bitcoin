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
class Context;

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
<<<<<<< HEAD
    void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) override
    {
        m_protocol->addCleanup(type, iface, std::move(cleanup));
    }
||||||| merged common ancestors
=======
    bool canConnect() override { return m_can_connect; }
    bool connectAddress(std::string& address, const MakeProxyFn& make_proxy) override
    {
        if (address.empty() || address == "0") return false;
        int fd = -1;
        std::string error;
        if (address == "auto") {
            // failure to connect with "auto" isn't an error. Caller can spawn a child process or just work offline.
            address = "unix";
            fd = m_process->connect(GetDataDir(), "bitcoin-node", address, error);
            if (fd < 0) return false;
        } else {
            fd = m_process->connect(GetDataDir(), "bitcoin-node", address, error);
        }
        if (fd < 0) {
            throw std::runtime_error(
                strprintf("Could not connect to bitcoin-node IPC address '%s'. %s", address, error));
        }
        std::unique_ptr<interfaces::Init> init = m_protocol->connect(fd);
        make_proxy(*init).addCloseHook(
            MakeUnique<interfaces::Deleter<std::unique_ptr<interfaces::Init>>>(std::move(init)));

        return true;
    }
    bool canListen() override { return m_can_listen; }
    bool listenAddress(std::string& address, std::string& error) override
    {
        int fd = m_process->bind(GetDataDir(), address, error);
        if (fd < 0) return false;
        m_protocol->listen(fd);
        return true;
    }
>>>>>>> multiprocess: Add -ipcconnect and -ipcbind options
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
