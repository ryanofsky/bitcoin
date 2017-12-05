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
    IpcImpl(int argc, char* argv[], const char* exe_name, interfaces::Init& init)
        : m_protocol(ipc::capnp::MakeCapnpProtocol(exe_name, init)),
          m_process(ipc::MakeProcess(argc, argv, exe_name, *m_protocol))
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
    Context& context() override { return m_protocol->context(); }
>>>>>>> Multiprocess bitcoin
    std::unique_ptr<Protocol> m_protocol;
    std::unique_ptr<Process> m_process;
};
} // namespace
} // namespace ipc

namespace interfaces {
std::unique_ptr<Ipc> MakeIpc(int argc, char* argv[], const char* exe_name, Init& init)
{
    return MakeUnique<ipc::IpcImpl>(argc, argv, exe_name, init);
}
} // namespace interfaces
