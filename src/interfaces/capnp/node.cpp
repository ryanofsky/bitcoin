// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/capnp/node.h>

#include <chainparams.h>
#include <init.h>
#include <interfaces/capnp/common-types.h>
#include <interfaces/capnp/common.h>
#include <interfaces/capnp/node-types.h>
#include <interfaces/capnp/node.capnp.h>
#include <interfaces/capnp/node.capnp.proxy-types.h>
#include <interfaces/capnp/node.capnp.proxy.h>
#include <interfaces/node.h>
#include <mp/proxy-io.h>
#include <mp/proxy-types.h>
#include <mp/util.h>
#include <rpc/server.h>
#include <util/memory.h>
#include <util/system.h>

#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

class CNodeStats;
struct CNodeStateStats;

namespace interfaces {
void MakeProxy(NodeClientParam& param)
{
    param.proxy = mp::MakeProxyClient<capnp::messages::Node, Node>(*param.context, kj::mv(*param.client));
}
void MakeProxy(NodeServerParam& param)
{
    param.proxy = mp::MakeProxyServer<capnp::messages::Node, Node>(*param.context, std::move(param.impl));
}

namespace capnp {
class RpcTimer : public ::RPCTimerBase
{
public:
    RpcTimer(mp::EventLoop& loop, std::function<void(void)>& fn, int64_t millis)
        : m_fn(fn), m_promise(loop.m_io_context.provider->getTimer()
                                  .afterDelay(millis * kj::MILLISECONDS)
                                  .then([this]() { m_fn(); })
                                  .eagerlyEvaluate(nullptr))
    {
    }
    ~RpcTimer() noexcept override {}

    std::function<void(void)> m_fn;
    kj::Promise<void> m_promise;
};

class RpcTimerInterface : public ::RPCTimerInterface
{
public:
    RpcTimerInterface(mp::EventLoop& loop) : m_loop(loop) {}
    const char* Name() override { return "Cap'n Proto"; }
    RPCTimerBase* NewTimer(std::function<void(void)>& fn, int64_t millis) override
    {
        RPCTimerBase* result;
        m_loop.sync([&] { result = new RpcTimer(m_loop, fn, millis); });
        return result;
    }
    mp::EventLoop& m_loop;
};
} // namespace capnp
} // namespace interfaces

namespace mp {
void ProxyServerMethodTraits<interfaces::capnp::messages::Node::RpcSetTimerInterfaceIfUnsetParams>::invoke(
    Context& context)
{
    if (!context.proxy_server.m_timer_interface) {
        auto timer = MakeUnique<interfaces::capnp::RpcTimerInterface>(context.proxy_server.m_connection.m_loop);
        context.proxy_server.m_timer_interface = std::move(timer);
    }
    context.proxy_server.m_impl->rpcSetTimerInterfaceIfUnset(context.proxy_server.m_timer_interface.get());
}

void ProxyServerMethodTraits<interfaces::capnp::messages::Node::RpcUnsetTimerInterfaceParams>::invoke(Context& context)
{
    context.proxy_server.m_impl->rpcUnsetTimerInterface(context.proxy_server.m_timer_interface.get());
    context.proxy_server.m_timer_interface.reset();
}

void CustomReadMessage(InvokeContext& invoke_context,
    interfaces::capnp::messages::NodeStats::Reader const& reader,
    std::tuple<CNodeStats, bool, CNodeStateStats>& node_stats)
{
    CNodeStats& node = std::get<0>(node_stats);
    ReadField(TypeList<CNodeStats>(), invoke_context, Make<ValueField>(reader), ReadDestValue(node));
    if ((std::get<1>(node_stats) = reader.hasStateStats())) {
        CNodeStateStats& state = std::get<2>(node_stats);
        ReadField(TypeList<CNodeStateStats>(), invoke_context, Make<ValueField>(reader.getStateStats()), ReadDestValue(state));
    }
}
} // namespace mp
