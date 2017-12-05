// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>

#include <interfaces/chain.h>
#include <interfaces/echo.h>
#include <interfaces/ipc.h>
#include <interfaces/node.h>
#include <interfaces/wallet.h>
#include <logging.h>
#include <node/context.h>
#include <util/memory.h>
#include <util/system.h>

#if defined(HAVE_CONFIG_H)
#include <config/bitcoin-config.h>
#endif

namespace interfaces {
namespace {
class LocalInitImpl : public LocalInit
{
public:
    LocalInitImpl() : LocalInit(/* exe_name= */ "bitcoind", /* log_suffix= */ nullptr)
    {
        m_node.args = &gArgs;
        m_node.init = this;
    }
    std::unique_ptr<Node> makeNode() override { return MakeNode(*this); }
    std::unique_ptr<Chain> makeChain() override { return MakeChain(m_node); }
    std::unique_ptr<WalletClient> makeWalletClient(Chain& chain) override
    {
#if ENABLE_WALLET
        return MakeWalletClient(chain, *Assert(m_node.args));
#else
        return {};
#endif
    }
    std::unique_ptr<Echo> makeEchoIpc() override
    {
        // The bitcoind binary isn't linked against libmultiprocess and doesn't
        // have IPC support, so just create a local interfaces::Echo object and
        // return it so the `echoipc` RPC method will work, and the python test
        // calling `echoipc` doesn't have to care whether it is testing a
        // bitcoind process without IPC support, or a bitcoin-node process with
        // IPC support.
        return MakeEcho();
    }
    NodeContext& node() override { return m_node; };
    NodeContext m_node;
};
} // namespace

std::unique_ptr<LocalInit> MakeInit(int argc, char* argv[]) { return MakeUnique<LocalInitImpl>(); }
} // namespace interfaces
