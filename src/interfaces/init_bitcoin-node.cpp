// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>

#include <interfaces/capnp/ipc.h>
#include <interfaces/echo.h>
#include <node/context.h>
#include <util/memory.h>

namespace interfaces {
namespace {
class LocalInitImpl : public LocalInit
{
public:
    LocalInitImpl(int argc, char* argv[]) : LocalInit(/* exe_name */ "bitcoin-node", /* log_suffix= */ nullptr)
    {
        m_node.init = this;
        m_protocol = capnp::MakeCapnpProtocol(m_exe_name, *this);
        m_process = MakeIpcProcess(argc, argv, m_exe_name, *m_protocol);
    }
    std::unique_ptr<Echo> makeEcho() override { return MakeEcho(); }
    std::unique_ptr<Echo> makeEchoIpc() override
    {
        // Spawn a new bitcoin-node process and call makeEcho to get a client
        // pointer to a interfaces::Echo instance running in that process. This
        // is just for testing. A slightly more realistic test would build a new
        // bitcoin-echo executable, and spawn bitcoin-echo here instead of
        // bitcoin-node. But using bitcoin-node avoids the need to build and
        // install a new binary not useful for anything except testing
        // multiprocess support.
        std::unique_ptr<Echo> echo;
        spawnProcess("bitcoin-node", [&](Init& init) -> Base& {
            echo = init.makeEcho();
            return *echo;
        });
        return echo;
    }
    NodeContext& node() override { return m_node; };
    NodeContext m_node;
};
} // namespace

std::unique_ptr<LocalInit> MakeInit(int argc, char* argv[])
{
    return MakeUnique<LocalInitImpl>(argc, argv);
}
} // namespace interfaces
