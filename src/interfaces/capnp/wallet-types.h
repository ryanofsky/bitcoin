// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_CAPNP_WALLET_TYPES_H
#define BITCOIN_INTERFACES_CAPNP_WALLET_TYPES_H

#include <interfaces/capnp/common-types.h>
#include <psbt.h>
#include <wallet/wallet.h>

class CCoinControl;
class CKey;

namespace mp {
void CustomBuildMessage(InvokeContext& invoke_context,
    const CTxDestination& dest,
    interfaces::capnp::messages::TxDestination::Builder&& builder);
void CustomReadMessage(InvokeContext& invoke_context,
    const interfaces::capnp::messages::TxDestination::Reader& reader,
    CTxDestination& dest);
void CustomBuildMessage(InvokeContext& invoke_context,
    const CKey& key,
    interfaces::capnp::messages::Key::Builder&& builder);
void CustomReadMessage(InvokeContext& invoke_context,
    const interfaces::capnp::messages::Key::Reader& reader,
    CKey& key);
void CustomBuildMessage(InvokeContext& invoke_context,
    const CCoinControl& coin_control,
    interfaces::capnp::messages::CoinControl::Builder&& builder);
void CustomReadMessage(InvokeContext& invoke_context,
    const interfaces::capnp::messages::CoinControl::Reader& reader,
    CCoinControl& coin_control);
} // namespace mp

#endif // BITCOIN_INTERFACES_CAPNP_WALLET_TYPES_H
