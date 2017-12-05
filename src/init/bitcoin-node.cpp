// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <node/context.h>
#include <util/memory.h>

#include <memory>

namespace init {
namespace {
const char* EXE_NAME = "bitcoin-node";

class BitcoinNodeInit : public interfaces::Init
{
public:
    BitcoinNodeInit(NodeContext& node, int argc, char* argv[])
        : m_node(node),
          m_ipc(interfaces::MakeIpc(argc, argv, EXE_NAME, *this))
    {
        m_node.init = this;
    }
    interfaces::Ipc* ipc() override { return m_ipc.get(); }
    NodeContext& m_node;
    std::unique_ptr<interfaces::Ipc> m_ipc;
};
} // namespace
} // namespace init

namespace interfaces {
std::unique_ptr<Init> MakeNodeInit(NodeContext& node, int argc, char* argv[], int& exit_status)
{
    auto init = MakeUnique<init::BitcoinNodeInit>(node, argc, argv);
    // Check if bitcoin-node is being invoked as an IPC server. If so, then
    // bypass normal execution and just respond to requests over the IPC
    // channel and return null.
    if (init->m_ipc->serveProcess(init::EXE_NAME, argc, argv, exit_status)) {
        return nullptr;
    }
    return init;
}
} // namespace interfaces
