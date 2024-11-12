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

<<<<<<< HEAD
    if (auto value{argsman.GetIntArg("-blockreconstructionextratxn")}) {
||||||| parent of b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
    if (auto value{argsman.GetIntArg("-maxorphantx")}) {
        options.max_orphan_txs = uint32_t((std::clamp<int64_t>(*value, 0, std::numeric_limits<uint32_t>::max())));
    }

    if (auto value{argsman.GetIntArg("-blockreconstructionextratxn")}) {
=======
    if (auto value{MaxOrphanTxSetting::Get(argsman)}) {
        options.max_orphan_txs = uint32_t((std::clamp<int64_t>(*value, 0, std::numeric_limits<uint32_t>::max())));
    }

    if (auto value{BlockReconstructionExtraTxnSetting::Get(argsman)}) {
>>>>>>> b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
        options.max_extra_txs = uint32_t((std::clamp<int64_t>(*value, 0, std::numeric_limits<uint32_t>::max())));
    }

    if (auto value{CaptureMessagesSetting::Get(argsman)}) options.capture_messages = *value;

    if (auto value{BlocksOnlySetting::Get(argsman)}) options.ignore_incoming_txs = *value;
}

} // namespace node

