// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_CAPNP_INIT_TYPES_H
#define BITCOIN_INTERFACES_CAPNP_INIT_TYPES_H

#include <interfaces/capnp/chain-types.h>
#include <interfaces/capnp/node-types.h>

namespace mp {
//! Specialization of makeWalletClient needed because it takes a Chain& reference
//! argument, not a unique_ptr<Chain> argument, so a manual addCloseHook()
//! callback is needed to clean up the ProxyServer<messages::Chain> proxy object.
template <>
struct ProxyServerMethodTraits<interfaces::capnp::messages::Init::MakeWalletClientParams>
{
    using Context = ServerContext<interfaces::capnp::messages::Init,
        interfaces::capnp::messages::Init::MakeWalletClientParams,
        interfaces::capnp::messages::Init::MakeWalletClientResults>;
    static std::unique_ptr<interfaces::ChainClient> invoke(Context& context,
        std::vector<std::string> wallet_filenames);
};
} // namespace mp

#endif // BITCOIN_INTERFACES_CAPNP_INIT_TYPES_H
