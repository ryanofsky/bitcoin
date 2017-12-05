// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/capnp/wallet.capnp.proxy-types.h>
#include <interfaces/capnp/wallet.capnp.proxy.h>

namespace mp {
void CustomBuildMessage(InvokeContext& invoke_context,
    const CTxDestination& dest,
    interfaces::capnp::messages::TxDestination::Builder&& builder)
{
    if (const PKHash* pkHash = boost::get<PKHash>(&dest)) {
        builder.setPkHash(interfaces::capnp::ToArray(interfaces::capnp::Serialize(*pkHash)));
    } else if (const ScriptHash* scriptHash = boost::get<ScriptHash>(&dest)) {
        builder.setScriptHash(interfaces::capnp::ToArray(interfaces::capnp::Serialize(*scriptHash)));
    } else if (const WitnessV0ScriptHash* witnessV0ScriptHash = boost::get<WitnessV0ScriptHash>(&dest)) {
        builder.setWitnessV0ScriptHash(interfaces::capnp::ToArray(interfaces::capnp::Serialize(*witnessV0ScriptHash)));
    } else if (const WitnessV0KeyHash* witnessV0KeyHash = boost::get<WitnessV0KeyHash>(&dest)) {
        builder.setWitnessV0KeyHash(interfaces::capnp::ToArray(interfaces::capnp::Serialize(*witnessV0KeyHash)));
    } else if (const WitnessUnknown* witnessUnknown = boost::get<WitnessUnknown>(&dest)) {
        BuildField(TypeList<WitnessUnknown>(), invoke_context, Make<ValueField>(builder.initWitnessUnknown()),
            *witnessUnknown);
    }
}

void CustomReadMessage(InvokeContext& invoke_context,
    const interfaces::capnp::messages::TxDestination::Reader& reader,
    CTxDestination& dest)
{
    if (reader.hasPkHash()) {
        dest = interfaces::capnp::Unserialize<PKHash>(reader.getPkHash());
    } else if (reader.hasScriptHash()) {
        dest = interfaces::capnp::Unserialize<ScriptHash>(reader.getScriptHash());
    } else if (reader.hasWitnessV0ScriptHash()) {
        dest = interfaces::capnp::Unserialize<WitnessV0ScriptHash>(reader.getWitnessV0ScriptHash());
    } else if (reader.hasWitnessV0KeyHash()) {
        dest = interfaces::capnp::Unserialize<WitnessV0KeyHash>(reader.getWitnessV0KeyHash());
    } else if (reader.hasWitnessUnknown()) {
        ReadFieldUpdate(TypeList<WitnessUnknown>(), invoke_context, Make<ValueField>(reader.getWitnessUnknown()),
            boost::get<WitnessUnknown>(dest));
    }
}

void CustomBuildMessage(InvokeContext& invoke_context,
    const CKey& key,
    interfaces::capnp::messages::Key::Builder&& builder)
{
    builder.setSecret(interfaces::capnp::FromBlob(key));
    builder.setIsCompressed(key.IsCompressed());
}

void CustomReadMessage(InvokeContext& invoke_context,
    const interfaces::capnp::messages::Key::Reader& reader,
    CKey& key)
{
    auto secret = reader.getSecret();
    key.Set(secret.begin(), secret.end(), reader.getIsCompressed());
}

void CustomBuildMessage(InvokeContext& invoke_context,
    const CCoinControl& coin_control,
    interfaces::capnp::messages::CoinControl::Builder&& builder)
{
    CustomBuildMessage(invoke_context, coin_control.destChange, builder.initDestChange());
    if (coin_control.m_change_type) {
        builder.setHasChangeType(true);
        builder.setChangeType(static_cast<int>(*coin_control.m_change_type));
    }
    builder.setAllowOtherInputs(coin_control.fAllowOtherInputs);
    builder.setAllowWatchOnly(coin_control.fAllowWatchOnly);
    builder.setOverrideFeeRate(coin_control.fOverrideFeeRate);
    if (coin_control.m_feerate) {
        builder.setFeeRate(interfaces::capnp::ToArray(interfaces::capnp::Serialize(*coin_control.m_feerate)));
    }
    if (coin_control.m_confirm_target) {
        builder.setHasConfirmTarget(true);
        builder.setConfirmTarget(*coin_control.m_confirm_target);
    }
    if (coin_control.m_signal_bip125_rbf) {
        builder.setHasSignalRbf(true);
        builder.setSignalRbf(*coin_control.m_signal_bip125_rbf);
    }
    builder.setFeeMode(int32_t(coin_control.m_fee_mode));
    builder.setMinDepth(coin_control.m_min_depth);
    std::vector<COutPoint> selected;
    coin_control.ListSelected(selected);
    auto builder_selected = builder.initSetSelected(selected.size());
    size_t i = 0;
    for (const COutPoint& output : selected) {
        builder_selected.set(i, interfaces::capnp::ToArray(interfaces::capnp::Serialize(output)));
        ++i;
    }
}

void CustomReadMessage(InvokeContext& invoke_context,
    const interfaces::capnp::messages::CoinControl::Reader& reader,
    CCoinControl& coin_control)
{
    CustomReadMessage(invoke_context, reader.getDestChange(), coin_control.destChange);
    if (reader.getHasChangeType()) {
        coin_control.m_change_type = OutputType(reader.getChangeType());
    }
    coin_control.fAllowOtherInputs = reader.getAllowOtherInputs();
    coin_control.fAllowWatchOnly = reader.getAllowWatchOnly();
    coin_control.fOverrideFeeRate = reader.getOverrideFeeRate();
    if (reader.hasFeeRate()) {
        coin_control.m_feerate = interfaces::capnp::Unserialize<CFeeRate>(reader.getFeeRate());
    }
    if (reader.getHasConfirmTarget()) {
        coin_control.m_confirm_target = reader.getConfirmTarget();
    }
    if (reader.getHasSignalRbf()) {
        coin_control.m_signal_bip125_rbf = reader.getSignalRbf();
    }
    coin_control.m_fee_mode = FeeEstimateMode(reader.getFeeMode());
    coin_control.m_min_depth = reader.getMinDepth();
    for (const auto output : reader.getSetSelected()) {
        coin_control.Select(interfaces::capnp::Unserialize<COutPoint>(output));
    }
}
} // namespace mp
