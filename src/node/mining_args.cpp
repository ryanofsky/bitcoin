// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/mining_args.h>

#include <common/args.h>
#include <common/messages.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <node/mining_types.h>
#include <policy/feerate.h>
#include <policy/policy.h>
#include <tinyformat.h>
#include <util/moneystr.h>
#include <util/result.h>
#include <util/translation.h>

#include <cstddef>
#include <optional>
#include <string>

using common::AmountErrMsg;

namespace node {

util::Result<void> CheckBlockMaxWeight(size_t block_max_weight, const std::string& arg_name)
{
    if (block_max_weight > MAX_BLOCK_WEIGHT) {
        return util::Error{Untranslated(strprintf("%s (%zu) exceeds consensus maximum block weight (%u)",
                                                  arg_name.empty() ? "block_max_weight" : arg_name,
                                                  block_max_weight, MAX_BLOCK_WEIGHT))};
    }
    return {};
}

util::Result<void> CheckBlockReservedWeight(size_t block_reserved_weight, const std::string& arg_name)
{
    if (block_reserved_weight < MINIMUM_BLOCK_RESERVED_WEIGHT) {
        return util::Error{Untranslated(strprintf("%s (%zu) is lower than minimum safety value of (%u)",
                                                  arg_name.empty() ? "block_reserved_weight" : arg_name,
                                                  block_reserved_weight, MINIMUM_BLOCK_RESERVED_WEIGHT))};
    }
    if (block_reserved_weight > MAX_BLOCK_WEIGHT) {
        return util::Error{Untranslated(strprintf("%s (%zu) exceeds consensus maximum block weight (%u)",
                                                  arg_name.empty() ? "block_reserved_weight" : arg_name,
                                                  block_reserved_weight, MAX_BLOCK_WEIGHT))};
    }
    return {};
}

util::Result<void> CheckCoinbaseOutputMaxAdditionalSigops(size_t sigops, const std::string& arg_name)
{
    if (sigops > MAX_BLOCK_SIGOPS_COST) {
        return util::Error{Untranslated(strprintf("%s (%zu) exceeds consensus maximum block sigops cost (%d)",
                                                  arg_name.empty() ? "coinbase_output_max_additional_sigops" : arg_name,
                                                  sigops, MAX_BLOCK_SIGOPS_COST))};
    }
    return {};
}

util::Result<void> ReadMiningArgs(const ArgsManager& args, BlockCreateOptions& options)
{
    if (const auto arg{args.GetArg("-blockmintxfee")}) {
        std::optional<CAmount> block_min_tx_fee{ParseMoney(*arg)};
        if (!block_min_tx_fee) return util::Error{AmountErrMsg("blockmintxfee", *arg)};
        options.block_min_fee_rate = CFeeRate{*block_min_tx_fee};
    } else {
        options.block_min_fee_rate = CFeeRate{DEFAULT_BLOCK_MIN_TX_FEE};
    }

    const size_t max_block_weight = args.GetIntArg("-blockmaxweight", DEFAULT_BLOCK_MAX_WEIGHT);
    if (auto result{CheckBlockMaxWeight(max_block_weight, "-blockmaxweight")}; !result) {
        return result;
    }
    options.block_max_weight = max_block_weight;

    const size_t block_reserved_weight = args.GetIntArg("-blockreservedweight", DEFAULT_BLOCK_RESERVED_WEIGHT);
    if (auto result{CheckBlockReservedWeight(block_reserved_weight, "-blockreservedweight")}; !result) {
        return result;
    }
    options.block_reserved_weight = block_reserved_weight;

    options.print_modified_fee = args.GetBoolArg("-printpriority", DEFAULT_PRINT_MODIFIED_FEE);

    return {};
}

util::Result<void> Check(const BlockCreateOptions& options)
{
    if (options.block_max_weight) {
        if (auto result{CheckBlockMaxWeight(*options.block_max_weight)}; !result) return result;
    }
    if (options.block_reserved_weight) {
        if (auto result{CheckBlockReservedWeight(*options.block_reserved_weight)}; !result) return result;
    }
    if (auto result{CheckCoinbaseOutputMaxAdditionalSigops(options.coinbase_output_max_additional_sigops)}; !result) {
        return result;
    }
    return {};
}

BlockCreateOptions Flatten(BlockCreateOptions options)
{
    if (!options.block_min_fee_rate) options.block_min_fee_rate = CFeeRate{DEFAULT_BLOCK_MIN_TX_FEE};
    if (!options.print_modified_fee) options.print_modified_fee = DEFAULT_PRINT_MODIFIED_FEE;
    if (!options.block_reserved_weight) options.block_reserved_weight = DEFAULT_BLOCK_RESERVED_WEIGHT;
    if (!options.block_max_weight) options.block_max_weight = DEFAULT_BLOCK_MAX_WEIGHT;
    return options;
}

BlockCreateOptions Merge(BlockCreateOptions x, const BlockCreateOptions& y)
{
    if (!x.block_min_fee_rate) x.block_min_fee_rate = y.block_min_fee_rate;
    if (!x.block_reserved_weight) x.block_reserved_weight = y.block_reserved_weight;
    if (!x.block_max_weight) x.block_max_weight = y.block_max_weight;
    if (!x.print_modified_fee) x.print_modified_fee = y.print_modified_fee;
    return x;
}

} // namespace node
