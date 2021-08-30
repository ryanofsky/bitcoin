// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init.h>
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
        ipc::capnp::SetupNodeClient(m_ipc->context());
    }
    interfaces::Ipc* ipc() override { return m_ipc.get(); }
<<<<<<< HEAD
    // bitcoin-gui accepts -ipcbind option even though it does not use it
    // directly. It just returns true here to accept the option because
    // bitcoin-node accepts the option, and bitcoin-gui accepts all bitcoin-node
    // options and will start the node with those options.
    bool canListenIpc() override { return true; }
    node::NodeContext m_node;
||||||| parent of 3a10e9a55cff (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
    node::NodeContext m_node;
=======
>>>>>>> 3a10e9a55cff (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
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
