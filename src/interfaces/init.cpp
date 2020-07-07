// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>

#include <interfaces/echo.h>
#include <interfaces/ipc.h>
#include <interfaces/node.h>
#include <logging.h>
#include <util/memory.h>

#include <tinyformat.h>

namespace interfaces {
namespace {
//! Close hook that encapsulate and deletes a moveable object.
class CloseFn : public CloseHook
{
public:
    explicit CloseFn(std::function<void()> fn) : m_fn(std::move(fn)) {}
    void onClose(Base& interface) override { m_fn(); }
    std::function<void()> m_fn;
};
} // namespace

LocalInit::LocalInit(const char* exe_name, const char* log_suffix) : m_exe_name(exe_name), m_log_suffix(log_suffix) {}
LocalInit::~LocalInit() {}
std::unique_ptr<Echo> LocalInit::makeEcho() { return {}; }
std::unique_ptr<Echo> LocalInit::makeEchoIpc() { return {}; }
std::unique_ptr<Node> LocalInit::makeNode() { return {}; }
std::unique_ptr<Chain> LocalInit::makeChain() { return {}; }
std::unique_ptr<ChainClient> LocalInit::makeWalletClient(Chain& chain, std::vector<std::string> wallet_filenames)
{
    return {};
}
NodeContext& LocalInit::node()
{
    throw std::logic_error("Node accessor function called from non-node binary (gui, wallet, or test program)");
}

void SpawnProcess(IpcProcess& process,
    IpcProtocol& protocol,
    const std::string& new_exe_name,
    const MakeClientFn& make_client)
{
    int pid;
    int fd = process.spawn(new_exe_name, pid);
    std::unique_ptr<Init> init = protocol.connect(fd);
    Base& base = make_client(*init);
    base.addCloseHook(MakeUnique<CloseFn>([&process, new_exe_name, pid] {
        int status = process.wait(pid);
        LogPrint(::BCLog::IPC, "%s pid %i exited with status %i\n", new_exe_name, pid, status);
    }));
    base.addCloseHook(MakeUnique<Deleter<std::unique_ptr<Init>>>(std::move(init)));
}

bool ConnectAddress(IpcProcess& process, IpcProtocol& protocol, const fs::path& data_dir, std::string& address, const MakeClientFn& make_client)
{
    if (address.empty() || address == "0") return false;
    int fd = -1;
    std::string error;
    if (address == "auto") {
        // failure to connect with "auto" isn't an error. Caller can spawn a child process or just work offline.
        address = "unix";
        fd = process.connect(data_dir, "bitcoin-node", address, error);
        if (fd < 0) return false;
    } else {
        fd = process.connect(data_dir, "bitcoin-node", address, error);
    }
    if (fd < 0) {
        throw std::runtime_error(strprintf("Could not connect to bitcoin-node IPC address '%s'. %s", address, error));
    }
    std::unique_ptr<Init> init = protocol.connect(fd);
    make_client(*init).addCloseHook(MakeUnique<Deleter<std::unique_ptr<Init>>>(std::move(init)));
    return true;
}
} // namespace interfaces
