// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <capnp/capability.h>
#include <interfaces/base.h>
#include <interfaces/chain.h>
#include <interfaces/init.h>
#include <interfaces/wallet.h>
#include <ipc/capnp/chain.capnp.h>
#include <ipc/capnp/chain.capnp.proxy.h>
#include <ipc/capnp/init-types.h>
#include <ipc/capnp/init.capnp.h>
#include <ipc/capnp/init.capnp.proxy.h>
#include <mp/proxy-io.h>
#include <util/memory.h>

#include <memory>
#include <utility>

namespace mp {
template <typename Interface> struct ProxyClient;

std::unique_ptr<interfaces::WalletClient>
ProxyServerMethodTraits<ipc::capnp::messages::Init::MakeWalletClientParams>::invoke(Context& context)
{
    auto params = context.call_context.getParams();
    auto chain = MakeUnique<ProxyClient<ipc::capnp::messages::Chain>>(
        params.getChain(), &context.proxy_server.m_connection, /* destroy_connection= */ false);
    auto client = context.proxy_server.m_impl->makeWalletClient(*chain);
    client->addCloseHook(MakeUnique<interfaces::Deleter<std::unique_ptr<interfaces::Chain>>>(std::move(chain)));
    return client;
}
} // namespace mp
