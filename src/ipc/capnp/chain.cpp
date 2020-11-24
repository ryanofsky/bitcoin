// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <capnp/blob.h>
#include <capnp/capability.h>
#include <capnp/list.h>
#include <coins.h>
#include <interfaces/base.h>
#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <ipc/capnp/chain-types.h>
#include <ipc/capnp/chain.capnp.h>
#include <ipc/capnp/chain.capnp.proxy.h>
#include <ipc/capnp/common-types.h>
#include <mp/proxy-io.h>
#include <mp/proxy-types.h>
#include <mp/util.h>
#include <primitives/block.h>
#include <rpc/server.h>
#include <streams.h>
#include <uint256.h>
#include <util/memory.h>

#include <assert.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace mp {
void CustomBuildMessage(InvokeContext& invoke_context,
                        const interfaces::FoundBlock& dest,
                        ipc::capnp::messages::FoundBlockParam::Builder&& builder)
{
    if (dest.m_hash) builder.setWantHash(true);
    if (dest.m_height) builder.setWantHeight(true);
    if (dest.m_time) builder.setWantTime(true);
    if (dest.m_max_time) builder.setWantMaxTime(true);
    if (dest.m_mtp_time) builder.setWantMtpTime(true);
    if (dest.m_data) builder.setWantData(true);
}

void CustomPassMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::FoundBlockParam::Reader& reader,
                       ipc::capnp::messages::FoundBlockResult::Builder&& builder,
                       std::function<void(const interfaces::FoundBlock&)>&& fn)
{
    interfaces::FoundBlock found_block;
    uint256 hash;
    int height = -1;
    int64_t time = -1;
    int64_t max_time = -1;
    int64_t mtp_time = -1;
    CBlock data;
    if (reader.getWantHash()) found_block.hash(hash);
    if (reader.getWantHeight()) found_block.height(height);
    if (reader.getWantTime()) found_block.time(time);
    if (reader.getWantMaxTime()) found_block.maxTime(max_time);
    if (reader.getWantMtpTime()) found_block.mtpTime(mtp_time);
    if (reader.getWantData()) found_block.data(data);
    fn(found_block);
    if (reader.getWantHash()) builder.setHash(ipc::capnp::ToArray(ipc::capnp::Serialize(hash)));
    if (reader.getWantHeight()) builder.setHeight(height);
    if (reader.getWantTime()) builder.setTime(time);
    if (reader.getWantMaxTime()) builder.setMaxTime(max_time);
    if (reader.getWantMtpTime()) builder.setMtpTime(mtp_time);
    if (reader.getWantData()) builder.setData(ipc::capnp::ToArray(ipc::capnp::Serialize(data)));
}

void CustomReadMessage(InvokeContext& invoke_context,
                       const ipc::capnp::messages::FoundBlockResult::Reader& reader,
                       const interfaces::FoundBlock& dest)
{
    if (dest.m_hash) *dest.m_hash = ipc::capnp::Unserialize<uint256>(reader.getHash());
    if (dest.m_height) *dest.m_height = reader.getHeight();
    if (dest.m_time) *dest.m_time = reader.getTime();
    if (dest.m_max_time) *dest.m_max_time = reader.getMaxTime();
    if (dest.m_mtp_time) *dest.m_mtp_time = reader.getMtpTime();
    if (dest.m_data) *dest.m_data = ipc::capnp::Unserialize<CBlock>(reader.getData());
}

std::unique_ptr<interfaces::Handler> ProxyServerMethodTraits<ipc::capnp::messages::Chain::HandleRpcParams>::invoke(
    Context& context)
{
    auto params = context.call_context.getParams();
    auto command = params.getCommand();

    CRPCCommand::Actor actor;
    ReadField(TypeList<decltype(actor)>(), context, Make<ValueField>(command.getActor()), ReadDestValue(actor));
    std::vector<std::string> args;
    ReadField(TypeList<decltype(args)>(), context, Make<ValueField>(command.getArgNames()), ReadDestValue(args));

    auto rpc_command = MakeUnique<CRPCCommand>(command.getCategory(), command.getName(), std::move(actor),
                                               std::move(args), command.getUniqueId());
    auto handler = context.proxy_server.m_impl->handleRpc(*rpc_command);
    handler->addCloseHook(MakeUnique<interfaces::Deleter<decltype(rpc_command)>>(std::move(rpc_command)));
    return handler;
}

void ProxyServerMethodTraits<ipc::capnp::messages::ChainClient::StartParams>::invoke(ChainContext& context)
{
    assert(0);
}

bool CustomHasValue(InvokeContext& invoke_context, const Coin& coin)
{
    // Spent coins cannot be serialized due to an assert in Coin::Serialize.
    return !coin.IsSpent();
}
} // namespace mp
