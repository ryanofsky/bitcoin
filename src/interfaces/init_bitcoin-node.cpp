// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>

#include <chainparams.h>
#include <interfaces/capnp/ipc.h>
#include <interfaces/chain.h>
#include <interfaces/node.h>
#include <node/context.h>

namespace interfaces {
void MakeProxy(NodeServerParam&);
namespace {
class LocalInitImpl : public LocalInit
{
public:
    LocalInitImpl(int argc, char* argv[]) : LocalInit(/* exe_name */ "bitcoin-node", /* log_suffix= */ nullptr)
    {
        m_protocol = capnp::MakeCapnpProtocol(m_exe_name, *this);
        m_process = MakeIpcProcess(argc, argv, m_exe_name, *m_protocol);
    }
    std::unique_ptr<Node> makeNode() override { return MakeNode(*this); }
    std::unique_ptr<Chain> makeChain() override { return MakeChain(m_node); }
    std::unique_ptr<ChainClient> makeWalletClient(Chain& chain, std::vector<std::string> wallet_filenames) override
    {
        std::unique_ptr<ChainClient> wallet;
        spawnProcess("bitcoin-wallet", [&](Init& init) -> Base& {
            wallet = init.makeWalletClient(chain, std::move(wallet_filenames));
            return *wallet;
        });
        return wallet;
    }
    void makeNodeServer(NodeServerParam& param) override { MakeProxy(param); }
    NodeContext& node() override { return m_node; };
    NodeContext m_node;
};
} // namespace

std::unique_ptr<LocalInit> MakeInit(int argc, char* argv[])
{
    return MakeUnique<LocalInitImpl>(argc, argv);
}
} // namespace interfaces
