// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/capnp/init.capnp.proxy-types.h>

namespace mp {

void CustomBuildMessage(InvokeContext& invoke_context,
                        const interfaces::FoundBlock& dest,
                        interfaces::capnp::messages::FoundBlockParam::Builder&& builder)
{
    if (dest.m_hash) builder.setWantHash(true);
    if (dest.m_height) builder.setWantHeight(true);
    if (dest.m_time) builder.setWantTime(true);
    if (dest.m_max_time) builder.setWantMaxTime(true);
    if (dest.m_mtp_time) builder.setWantMtpTime(true);
    if (dest.m_data) builder.setWantData(true);
}

void CustomPassMessage(InvokeContext& invoke_context,
                       const interfaces::capnp::messages::FoundBlockParam::Reader& reader,
                       interfaces::capnp::messages::FoundBlockResult::Builder&& builder,
                       std::function<void(const interfaces::FoundBlock&)>&& fn)
{
    interfaces::FoundBlock found_block;
    uint256 hash;
    int height;
    int64_t time;
    int64_t max_time;
    int64_t mtp_time;
    CBlock data;
    if (reader.getWantHash()) found_block.hash(hash);
    if (reader.getWantHeight()) found_block.height(height);
    if (reader.getWantTime()) found_block.time(time);
    if (reader.getWantMaxTime()) found_block.maxTime(max_time);
    if (reader.getWantMtpTime()) found_block.mtpTime(mtp_time);
    if (reader.getWantData()) found_block.data(data);
    fn(found_block);
    if (reader.getWantHash()) builder.setHash(interfaces::capnp::ToArray(interfaces::capnp::Serialize(hash)));
    if (reader.getWantHeight()) builder.setHeight(height);
    if (reader.getWantTime()) builder.setTime(time);
    if (reader.getWantMaxTime()) builder.setMaxTime(max_time);
    if (reader.getWantMtpTime()) builder.setMtpTime(mtp_time);
    if (reader.getWantData()) builder.setData(interfaces::capnp::ToArray(interfaces::capnp::Serialize(data)));
}

void CustomReadMessage(InvokeContext& invoke_context,
                       const interfaces::capnp::messages::FoundBlockResult::Reader& reader,
                       const interfaces::FoundBlock& dest)
{
    if (dest.m_hash) *dest.m_hash = interfaces::capnp::Unserialize<uint256>(reader.getHash());
    if (dest.m_height) *dest.m_height = reader.getHeight();
    if (dest.m_time) *dest.m_time = reader.getTime();
    if (dest.m_max_time) *dest.m_max_time = reader.getMaxTime();
    if (dest.m_mtp_time) *dest.m_mtp_time = reader.getMtpTime();
    if (dest.m_data) *dest.m_data = interfaces::capnp::Unserialize<CBlock>(reader.getData());
}

std::unique_ptr<interfaces::Handler>
ProxyServerMethodTraits<interfaces::capnp::messages::Chain::HandleRpcParams>::invoke(Context& context)
{
    auto params = context.call_context.getParams();
    auto command = params.getCommand();

    CRPCCommand::Actor actor;
    ReadField(TypeList<decltype(actor)>(), context, Make<ValueField>(command.getActor()), ReadDestValue(actor));
    std::vector<std::string> args;
    ReadField(TypeList<decltype(args)>(), context, Make<ValueField>(command.getArgNames()), ReadDestValue(args));

    auto rpc_command = MakeUnique<CRPCCommand>(
        command.getCategory(), command.getName(), std::move(actor), std::move(args), command.getUniqueId());
    auto handler = context.proxy_server.m_impl->handleRpc(*rpc_command);
    handler->addCloseHook(MakeUnique<interfaces::Deleter<decltype(rpc_command)>>(std::move(rpc_command)));
    return handler;
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
