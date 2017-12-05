// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/capnp/common.capnp.proxy-types.h>

namespace mp {

void CustomBuildMessage(InvokeContext& invoke_context,
    const UniValue& univalue,
    interfaces::capnp::messages::UniValue::Builder&& builder)
{
    builder.setType(univalue.getType());
    if (univalue.getType() == UniValue::VARR || univalue.getType() == UniValue::VOBJ) {
        builder.setValue(univalue.write());
    } else {
        builder.setValue(univalue.getValStr());
    }
}

void CustomReadMessage(InvokeContext& invoke_context,
    const interfaces::capnp::messages::UniValue::Reader& reader,
    UniValue& univalue)
{
    if (reader.getType() == UniValue::VARR || reader.getType() == UniValue::VOBJ) {
        if (!univalue.read(interfaces::capnp::ToString(reader.getValue()))) {
            throw std::runtime_error("Could not parse UniValue");
        }
    } else {
        univalue = UniValue(UniValue::VType(reader.getType()), interfaces::capnp::ToString(reader.getValue()));
    }
}

} // namespace mp

namespace interfaces {
namespace capnp {

void BuildGlobalArgs(mp::InvokeContext& invoke_context, messages::GlobalArgs::Builder&& builder)
{

    const auto& args = static_cast<const GlobalArgs&>(::gArgs);
    LOCK(args.cs_args);
    mp::BuildField(mp::TypeList<GlobalArgs>(), invoke_context, mp::Make<mp::ValueField>(builder), args);
}

void ReadGlobalArgs(mp::InvokeContext& invoke_context, const messages::GlobalArgs::Reader& reader)
{
    auto& args = static_cast<GlobalArgs&>(::gArgs);
    {
        LOCK(args.cs_args);
        mp::ReadFieldUpdate(mp::TypeList<GlobalArgs>(), invoke_context, mp::Make<mp::ValueField>(reader), args);
    }
    interfaces::LocalInit& init = *static_cast<interfaces::LocalInit*>(invoke_context.connection.m_loop.m_context);
    init.initProcess();
}

std::string GlobalArgsNetwork()
{
    auto& args = static_cast<GlobalArgs&>(::gArgs);
    LOCK(args.cs_args);
    return args.m_network;
}

} // namespace capnp
} // namespace interfaces
