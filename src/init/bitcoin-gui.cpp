// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fs.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <ipc/context.h>
#include <logging.h>
#include <tinyformat.h>
#include <util/memory.h>
#include <util/time.h>

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

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
<<<<<<< HEAD
    BitcoinGuiInit(const char* arg0) : m_ipc(interfaces::MakeIpc(EXE_NAME, arg0, *this))
||||||| parent of 871891d38cb (multiprocess: Add bitcoin-gui -ipcconnect option)
    BitcoinGuiInit(const char* arg0)
        : m_ipc(interfaces::MakeIpc(EXE_NAME, arg0, *this, /* can_connect= */ false, /* can_listen= */ true))
=======
    BitcoinGuiInit(const char* arg0)
        : m_ipc(interfaces::MakeIpc(EXE_NAME, arg0, *this, /* can_connect= */ true, /* can_listen= */ true))
>>>>>>> 871891d38cb (multiprocess: Add bitcoin-gui -ipcconnect option)
    {
        ipc::capnp::SetupNodeClient(m_ipc->context());
    }
    interfaces::Ipc* ipc() override { return m_ipc.get(); }
    bool canListenIpc() override { return true; }
    std::unique_ptr<interfaces::Ipc> m_ipc;
};
} // namespace
} // namespace init

namespace interfaces {
std::unique_ptr<Init> MakeGuiInit(int argc, char* argv[])
{
    return MakeUnique<init::BitcoinGuiInit>(argc > 0 ? argv[0] : "");
}
} // namespace interfaces
