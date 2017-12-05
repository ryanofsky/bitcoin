// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_CAPNP_CHAIN_TYPES_H
#define BITCOIN_INTERFACES_CAPNP_CHAIN_TYPES_H

#include <interfaces/capnp/common-types.h>
#include <interfaces/capnp/common.capnp.proxy-types.h>
#include <rpc/server.h>

//! Specialization of handleRpc needed because it takes a CRPCCommand& reference
//! argument, so a manual addCloseHook() callback is needed to free the passed
//! CRPCCommand struct and proxy ActorCallback object.
template <>
struct mp::ProxyServerMethodTraits<interfaces::capnp::messages::Chain::HandleRpcParams>
{
    using Context = ServerContext<interfaces::capnp::messages::Chain,
        interfaces::capnp::messages::Chain::HandleRpcParams,
        interfaces::capnp::messages::Chain::HandleRpcResults>;
    static std::unique_ptr<interfaces::Handler> invoke(Context& context);
};

//! Specialization of start method needed to provide CScheduler& reference
//! argument.
template <>
struct mp::ProxyServerMethodTraits<interfaces::capnp::messages::ChainClient::StartParams>
{
    using Context = ServerContext<interfaces::capnp::messages::ChainClient,
        interfaces::capnp::messages::ChainClient::StartParams,
        interfaces::capnp::messages::ChainClient::StartResults>;
    static void invoke(Context& context);
};

namespace mp {
void CustomBuildMessage(InvokeContext& invoke_context,
                        const interfaces::FoundBlock& dest,
                        interfaces::capnp::messages::FoundBlockParam::Builder&& builder);
void CustomPassMessage(InvokeContext& invoke_context,
                        const interfaces::capnp::messages::FoundBlockParam::Reader& reader,
                        interfaces::capnp::messages::FoundBlockResult::Builder&& builder,
                        std::function<void(const interfaces::FoundBlock&)>&& fn);
void CustomReadMessage(InvokeContext& invoke_context,
                       const interfaces::capnp::messages::FoundBlockResult::Reader& reader,
                       const interfaces::FoundBlock& dest);

//! JSONRPCRequest& server-side argument handling. Needed because JSONRPCRequest
//! doesn't have a default constructor.
template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
void CustomPassField(TypeList<const JSONRPCRequest&>, ServerContext& server_context, Fn&& fn, Args&&... args)
{
    util::Ref context;
    JSONRPCRequest request(context);
    const auto& params = server_context.call_context.getParams();
    ReadField(TypeList<JSONRPCRequest>(), server_context, Make<StructField, Accessor>(params), ReadDestValue(request));
    fn.invoke(server_context, std::forward<Args>(args)..., request);
}

//! CScheduler& server-side argument handling. Skips argument so it can
//! be handled by ProxyServerCustom code.
template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
void CustomPassField(TypeList<CScheduler&>, ServerContext& server_context, const Fn& fn, Args&&... args)
{
    fn.invoke(server_context, std::forward<Args>(args)...);
}

//! CRPCCommand& server-side argument handling. Skips argument so it can
//! be handled by ProxyServerCustom code.
template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
void CustomPassField(TypeList<const CRPCCommand&>, ServerContext& server_context, const Fn& fn, Args&&... args)
{
    fn.invoke(server_context, std::forward<Args>(args)...);
}

//! Chain& server-side argument handling. Skips argument so it can
//! be handled by ProxyServerCustom code.
template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
void CustomPassField(TypeList<interfaces::Chain&>, ServerContext& server_context, const Fn& fn, Args&&... args)
{
    fn.invoke(server_context, std::forward<Args>(args)...);
}

bool CustomHasValue(InvokeContext& invoke_context, const Coin& coin);
} // namespace mp

#endif // BITCOIN_INTERFACES_CAPNP_CHAIN_TYPES_H
