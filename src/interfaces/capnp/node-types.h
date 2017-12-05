// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_CAPNP_NODE_TYPES_H
#define BITCOIN_INTERFACES_CAPNP_NODE_TYPES_H

#include <interfaces/capnp/common-types.h>
#include <interfaces/init.h>

class CNodeStats;
struct CNodeStateStats;

namespace interfaces {
struct NodeClientParam
{
    mp::InvokeContext* context;
    capnp::messages::Node::Client* client;
    std::unique_ptr<Node> proxy;
};
struct NodeServerParam
{
    mp::InvokeContext* context;
    std::shared_ptr<Node> impl;
    kj::Own<capnp::messages::Node::Server> proxy;
};
} // namespace interfaces

//! Specialization of rpcSetTimerInterfaceIfUnset needed because it takes a
//! RPCTimerInterface* argument, which requires custom code to provide a
//! compatible timer.
template <>
struct mp::ProxyServerMethodTraits<interfaces::capnp::messages::Node::RpcSetTimerInterfaceIfUnsetParams>
{
    using Context = ServerContext<interfaces::capnp::messages::Node,
        interfaces::capnp::messages::Node::RpcSetTimerInterfaceIfUnsetParams,
        interfaces::capnp::messages::Node::RpcSetTimerInterfaceIfUnsetResults>;
    static void invoke(Context& context);
};

//! Specialization of rpcUnsetTimerInterface needed because it takes a
//! RPCTimerInterface* argument, which requires custom code to provide a
//! compatible timer.
template <>
struct mp::ProxyServerMethodTraits<interfaces::capnp::messages::Node::RpcUnsetTimerInterfaceParams>
{
    using Context = ServerContext<interfaces::capnp::messages::Node,
        interfaces::capnp::messages::Node::RpcUnsetTimerInterfaceParams,
        interfaces::capnp::messages::Node::RpcUnsetTimerInterfaceResults>;
    static void invoke(Context& context);
};

namespace mp {
//! Specialization of MakeProxyClient for Node to that constructs a client
//! object through a function pointer so client object code relying on
//! net_processing types doesn't need to get linked into the bitcoin-wallet
//! executable.
template <>
inline std::unique_ptr<interfaces::Node> CustomMakeProxyClient<interfaces::capnp::messages::Node, interfaces::Node>(
    InvokeContext& context,
    interfaces::capnp::messages::Node::Client&& client)
{
    interfaces::NodeClientParam param;
    param.context = &context;
    param.client = &client;
    interfaces::LocalInit& init = *static_cast<interfaces::LocalInit*>(context.connection.m_loop.m_context);
    init.makeNodeClient(param);
    return std::move(param.proxy);
}

//! Specialization of MakeProxyServer for Node to that constructs a server
//! object through a function pointer so server object code relying on
//! net_processing types doesn't need to get linked into the bitcoin-wallet
//! executable.
template <>
inline kj::Own<interfaces::capnp::messages::Node::Server>
CustomMakeProxyServer<interfaces::capnp::messages::Node, interfaces::Node>(InvokeContext& context,
    std::shared_ptr<interfaces::Node>&& impl)
{
    interfaces::NodeServerParam param;
    param.context = &context;
    param.impl = std::move(impl);
    interfaces::LocalInit& init = *static_cast<interfaces::LocalInit*>(context.connection.m_loop.m_context);
    init.makeNodeServer(param);
    return kj::mv(param.proxy);
}

template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
void CustomPassField(TypeList<int, const char* const*>, ServerContext& server_context, const Fn& fn, Args&&... args)
{
    const auto& params = server_context.call_context.getParams();
    const auto& value = Accessor::get(params);
    std::vector<const char*> argv(value.size());
    size_t i = 0;
    for (const auto arg : value) {
        argv[i] = arg.cStr();
        ++i;
    }
    return fn.invoke(server_context, std::forward<Args>(args)..., argv.size(), argv.data());
}

template <typename Output>
void CustomBuildField(TypeList<int, const char* const*>,
    Priority<1>,
    InvokeContext& invoke_context,
    int argc,
    const char* const* argv,
    Output&& output)
{
    auto args = output.init(argc);
    for (int i = 0; i < argc; ++i) {
        args.set(i, argv[i]);
    }
}

template <typename InvokeContext>
static inline ::capnp::Void BuildPrimitive(InvokeContext& invoke_context, RPCTimerInterface*, TypeList<::capnp::Void>)
{
    return {};
}

//! RPCTimerInterface* server-side argument handling. Skips argument so it can
//! be handled by ProxyServerCustom code.
template <typename Accessor, typename ServerContext, typename Fn, typename... Args>
void CustomPassField(TypeList<RPCTimerInterface*>, ServerContext& server_context, const Fn& fn, Args&&... args)
{
    fn.invoke(server_context, std::forward<Args>(args)...);
}

template <typename Value, typename Output>
void CustomBuildField(TypeList<std::tuple<CNodeStats, bool, CNodeStateStats>>,
    Priority<1>,
    InvokeContext& invoke_context,
    Value&& stats,
    Output&& output)
{
    // FIXME should pass message_builder instead of builder below to avoid
    // calling output.set twice Need ValueBuilder class analogous to
    // ValueReader for this
    BuildField(TypeList<CNodeStats>(), invoke_context, output, std::get<0>(stats));
    if (std::get<1>(stats)) {
        auto message_builder = output.init();
        using Accessor = ProxyStruct<interfaces::capnp::messages::NodeStats>::StateStatsAccessor;
        StructField<Accessor, interfaces::capnp::messages::NodeStats::Builder> field_output{message_builder};
        BuildField(TypeList<CNodeStateStats>(), invoke_context, field_output, std::get<2>(stats));
    }
}

void CustomReadMessage(InvokeContext& invoke_context,
    interfaces::capnp::messages::NodeStats::Reader const& reader,
    std::tuple<CNodeStats, bool, CNodeStateStats>& node_stats);
} // namespace mp

#endif // BITCOIN_INTERFACES_CAPNP_NODE_TYPES_H
