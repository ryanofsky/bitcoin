// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/capnp/init.capnp.proxy-types.h>

namespace mp {

std::unique_ptr<interfaces::Handler>
ProxyServerMethodTraits<interfaces::capnp::messages::Chain::HandleNotificationsParams>::invoke(Context& context)
{
    auto params = context.call_context.getParams();
    auto notifications = MakeUnique<ProxyClient<interfaces::capnp::messages::ChainNotifications>>(
        params.getNotifications(), &context.proxy_server.m_connection, /* destroy_connection= */ false);
    auto handler = context.proxy_server.m_impl->handleNotifications(*notifications);
    handler->addCloseHook(MakeUnique<interfaces::Deleter<decltype(notifications)>>(std::move(notifications)));
    return handler;
}

void ProxyServerMethodTraits<interfaces::capnp::messages::Chain::RequestMempoolTransactionsParams>::invoke(
    Context& context)
{
    auto params = context.call_context.getParams();
    auto notifications = MakeUnique<ProxyClient<interfaces::capnp::messages::ChainNotifications>>(
        params.getNotifications(), &context.proxy_server.m_connection, /* destroy_connection= */ false);
    context.proxy_server.m_impl->requestMempoolTransactions(*notifications);
}

std::unique_ptr<interfaces::Handler>
ProxyServerMethodTraits<interfaces::capnp::messages::Chain::HandleRpcParams>::invoke(Context& context)
{
    auto params = context.call_context.getParams();
    auto command = params.getCommand();

    CRPCCommand::Actor actor;
    ReadFieldUpdate(TypeList<decltype(actor)>(), context, Make<ValueField>(command.getActor()), actor);
    std::vector<std::string> args;
    ReadFieldUpdate(TypeList<decltype(args)>(), context, Make<ValueField>(command.getArgNames()), args);

    auto rpc_command = MakeUnique<CRPCCommand>(
        command.getCategory(), command.getName(), std::move(actor), std::move(args), command.getUniqueId());
    auto handler = context.proxy_server.m_impl->handleRpc(*rpc_command);
    handler->addCloseHook(MakeUnique<interfaces::Deleter<decltype(rpc_command)>>(std::move(rpc_command)));
    return handler;
}

ProxyServerCustom<interfaces::capnp::messages::ChainClient, interfaces::ChainClient>::ProxyServerCustom(
    interfaces::ChainClient* impl,
    bool owned,
    Connection& connection)
    : ProxyServerBase(impl, owned, connection)
{
}

void ProxyServerCustom<interfaces::capnp::messages::ChainClient, interfaces::ChainClient>::invokeDestroy()
{
    if (m_scheduler) {
        m_scheduler->stop();
        m_result.get();
        m_scheduler.reset();
    }
    ProxyServerBase::invokeDestroy();
}

void ProxyServerMethodTraits<interfaces::capnp::messages::ChainClient::StartParams>::invoke(Context& context)
{
    if (!context.proxy_server.m_scheduler) {
        context.proxy_server.m_scheduler = MakeUnique<CScheduler>();
        CScheduler* scheduler = context.proxy_server.m_scheduler.get();
        context.proxy_server.m_result = std::async([scheduler]() {
            util::ThreadRename("schedqueue");
            scheduler->serviceQueue();
        });
    }
    context.proxy_server.m_impl->start(*context.proxy_server.m_scheduler);
}

bool CustomHasValue(InvokeContext& invoke_context, const Coin& coin)
{
    // Spent coins cannot be serialized due to an assert in Coin::Serialize.
    return !coin.IsSpent();
}

} // namespace mp
