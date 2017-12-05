// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>

#include <chainparams.h>
#include <init.h>
#include <interfaces/capnp/ipc.h>
#include <interfaces/echo.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <node/context.h>
#include <util/memory.h>
#include <util/system.h>

namespace interfaces {
void MakeProxy(NodeServerParam&);
namespace capnp {
std::string GlobalArgsNetwork();
} // namespace capnp
namespace {
class LocalInitImpl : public LocalInit
{
public:
    LocalInitImpl(int argc, char* argv[]) : LocalInit(/* exe_name */ "bitcoin-node", /* log_suffix= */ nullptr)
    {
        m_node.args = &gArgs;
        m_node.init = this;
        m_protocol = capnp::MakeCapnpProtocol(m_exe_name, *this);
        m_process = MakeIpcProcess(argc, argv, m_exe_name, *m_protocol);
    }
    std::unique_ptr<Echo> makeEcho() override { return MakeEcho(); }
    std::unique_ptr<Node> makeNode() override { return MakeNode(*this); }
    std::unique_ptr<Chain> makeChain() override { return MakeChain(m_node); }
    std::unique_ptr<WalletClient> makeWalletClient(Chain& chain) override
    {
        std::unique_ptr<WalletClient> wallet;
        SpawnProcess(*m_process, *m_protocol, "bitcoin-wallet", [&](Init& init) -> Base& {
            wallet = init.makeWalletClient(chain);
            return *wallet;
        });
        return wallet;
    }
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
        SpawnProcess(*m_process, *m_protocol, "bitcoin-node", [&](Init& init) -> Base& {
            echo = init.makeEcho();
            return *echo;
        });
        return echo;
    }
    void initProcess() override
    {
        // TODO in future PR: Refactor bitcoin startup code, dedup this with AppInit.
        SelectParams(interfaces::capnp::GlobalArgsNetwork());
        InitLogging(*Assert(m_node.args));
        InitParameterInteraction(*Assert(m_node.args));
    }
    void makeNodeServer(NodeServerParam& param) override { MakeProxy(param); }
    NodeContext& node() override { return m_node; };
    NodeContext m_node;
};
} // namespace

std::unique_ptr<LocalInit> MakeInit(int argc, char* argv[]) { return MakeUnique<LocalInitImpl>(argc, argv); }
} // namespace interfaces
