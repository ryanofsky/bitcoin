// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <init.h>
#include <interfaces/capnp/init-types.h>
#include <interfaces/init.h>

namespace mp {
std::unique_ptr<interfaces::WalletClient>
ProxyServerMethodTraits<interfaces::capnp::messages::Init::MakeWalletClientParams>::invoke(Context& context,
    std::vector<std::string> wallet_filenames)
{
    auto params = context.call_context.getParams();
    auto chain = MakeUnique<ProxyClient<interfaces::capnp::messages::Chain>>(
        params.getChain(), &context.proxy_server.m_connection, /* destroy_connection= */ false);
    auto client = context.proxy_server.m_impl->makeWalletClient(*chain, std::move(wallet_filenames));
    client->addCloseHook(MakeUnique<interfaces::Deleter<std::unique_ptr<interfaces::Chain>>>(std::move(chain)));
    return client;
}
} // namespace mp
