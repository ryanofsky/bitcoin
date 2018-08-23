// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
#include <interfaces/base.h>
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
//! Close hook that encapsulates a function to be called on close.
class CloseFn : public interfaces::CloseHook
{
public:
    explicit CloseFn(std::function<void()> fn) : m_fn(std::move(fn)) {}
    void onClose(interfaces::Base& interface) override { m_fn(); }
    std::function<void()> m_fn;
};

class IpcImpl : public interfaces::Ipc
{
public:
    IpcImpl(int argc, char* argv[], const char* exe_name, interfaces::Init& init, bool can_connect, bool can_listen)
        : m_protocol(ipc::capnp::MakeCapnpProtocol(exe_name, init)),
          m_process(ipc::MakeProcess(argc, argv, exe_name, *m_protocol)), m_can_connect(can_connect),
          m_can_listen(can_listen)
    {
    }
    void spawnProcess(const char* exe_name, const MakeProxyFn& make_proxy) override
    {
        int pid;
        int fd = m_process->spawn(exe_name, pid);
        LogPrint(::BCLog::IPC, "Process %s pid %i launched\n", exe_name, pid);
        std::unique_ptr<interfaces::Init> init = m_protocol->connect(fd);
        interfaces::Base& base = make_proxy(*init);
        base.addCloseHook(MakeUnique<CloseFn>([this, exe_name, pid] {
            int status = m_process->wait(pid);
            LogPrint(::BCLog::IPC, "Process %s pid %i exited with status %i\n", exe_name, pid, status);
        }));
        base.addCloseHook(MakeUnique<interfaces::Deleter<std::unique_ptr<interfaces::Init>>>(std::move(init)));
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
