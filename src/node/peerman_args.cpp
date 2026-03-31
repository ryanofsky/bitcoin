// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://opensource.org/license/mit.

#include <node/peerman_args.h>

#include <common/args.h>
#include <init_settings.h>
#include <net_processing.h>

#include <algorithm>
#include <limits>

namespace node {

void ApplyArgsManOptions(const ArgsManager& argsman, PeerManager::Options& options)
{
    if (auto value{TxReconciliationSetting::Get(argsman)}) options.reconcile_txs = *value;

    if (auto value{BlockReconstructionExtraTxnSetting::Get(argsman)}) {
        options.max_extra_txs = uint32_t((std::clamp<int64_t>(*value, 0, std::numeric_limits<uint32_t>::max())));
    }

    if (auto value{CaptureMessagesSetting::Get(argsman)}) options.capture_messages = *value;

    if (auto value{BlocksOnlySetting::Get(argsman)}) options.ignore_incoming_txs = *value;

<<<<<<< HEAD
    if (auto value{argsman.GetIntArg("-txsendrate")}) {
        options.tx_send_rate = uint32_t(std::clamp<int64_t>(*value, 1, 1000));
    }

    if (auto value{argsman.GetBoolArg("-privatebroadcast")}) options.private_broadcast = *value;
||||||| parent of 0612ffcb492 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
    if (auto value{argsman.GetBoolArg("-privatebroadcast")}) options.private_broadcast = *value;
=======
    if (auto value{PrivatebroadcastSetting::Get(argsman)}) options.private_broadcast = *value;
>>>>>>> 0612ffcb492 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
}

} // namespace node

