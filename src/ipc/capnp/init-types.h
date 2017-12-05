// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_INIT_TYPES_H
#define BITCOIN_IPC_CAPNP_INIT_TYPES_H

#include <ipc/capnp/chain-types.h>
#include <ipc/capnp/echo.capnp.proxy-types.h>
#include <ipc/capnp/node-types.h>

namespace mp {
//! Specialization of makeWalletClient needed because it takes a Chain& reference
//! argument, not a unique_ptr<Chain> argument, so a manual addCloseHook()
//! callback is needed to clean up the ProxyServer<messages::Chain> proxy object.
template <>
struct ProxyServerMethodTraits<ipc::capnp::messages::Init::MakeWalletClientParams>
{
    using Context = ServerContext<ipc::capnp::messages::Init,
                                  ipc::capnp::messages::Init::MakeWalletClientParams,
                                  ipc::capnp::messages::Init::MakeWalletClientResults>;
    static std::unique_ptr<interfaces::WalletClient> invoke(Context& context);
};
} // namespace mp

#endif // BITCOIN_IPC_CAPNP_INIT_TYPES_H
