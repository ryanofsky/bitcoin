// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

<<<<<<< HEAD
#include <init.h>
#include <interfaces/chain.h>
#include <interfaces/echo.h>
||||||| parent of ce94dd1a81ad (Make bitcoin-gui spawn a bitcoin-node process)
#include <common/args.h>
#include <interfaces/chain.h>
#include <interfaces/echo.h>
=======
#include <common/args.h>
>>>>>>> ce94dd1a81ad (Make bitcoin-gui spawn a bitcoin-node process)
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <util/check.h>

#include <memory>

namespace ipc {
namespace capnp {
void SetupNodeClient(ipc::Context& context);
} // namespace capnp
} // namespace ipc

namespace init {
namespace {
const char* EXE_NAME = "bitcoin-gui";

class BitcoinGuiInit : public interfaces::Init
{
public:
    BitcoinGuiInit(const char* arg0) : m_ipc(interfaces::MakeIpc(EXE_NAME, ".gui", arg0, *this))
    {
<<<<<<< HEAD
        InitContext(m_node);
        m_node.init = this;
||||||| parent of ce94dd1a81ad (Make bitcoin-gui spawn a bitcoin-node process)
        m_node.args = &gArgs;
        m_node.init = this;
=======
        ipc::capnp::SetupNodeClient(m_ipc->context());
>>>>>>> ce94dd1a81ad (Make bitcoin-gui spawn a bitcoin-node process)
    }
    interfaces::Ipc* ipc() override { return m_ipc.get(); }
    std::unique_ptr<interfaces::Ipc> m_ipc;
};
} // namespace
} // namespace init

namespace interfaces {
std::unique_ptr<Init> MakeGuiInit(int argc, char* argv[])
{
    return std::make_unique<init::BitcoinGuiInit>(argc > 0 ? argv[0] : "");
}
} // namespace interfaces
