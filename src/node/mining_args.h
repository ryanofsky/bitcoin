// Copyright (c) 2026 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_MINING_ARGS_H
#define BITCOIN_NODE_MINING_ARGS_H

#include <node/mining_types.h>
#include <util/result.h>

#include <cstddef>
#include <string>

class ArgsManager;

namespace node {

static const bool DEFAULT_PRINT_MODIFIED_FEE = false;

/**
 * Overlay the options set in \p args on top of corresponding members in
 * \p options. Returns an error if one was encountered.
 *
 * @param[in]  args The ArgsManager in which to check set options.
 * @param[in,out] options The BlockCreateOptions to modify according to \p args.
 */
[[nodiscard]] util::Result<void> ReadMiningArgs(const ArgsManager& args, BlockCreateOptions& options);

/** Check option values for validity. Returns an error for invalid values. */
[[nodiscard]] util::Result<void> Check(const BlockCreateOptions& options);

/** Replace null optional values with their hardcoded defaults. */
[[nodiscard]] BlockCreateOptions Flatten(BlockCreateOptions options);

/**
 * Merge two BlockCreateOptions structs, replacing null values in \p x with
 * non-null values from \p y.
 */
[[nodiscard]] BlockCreateOptions Merge(BlockCreateOptions x, const BlockCreateOptions& y);

/** Check that block_max_weight does not exceed consensus limits. */
[[nodiscard]] util::Result<void> CheckBlockMaxWeight(size_t block_max_weight, const std::string& arg_name = "");

/** Check that block_reserved_weight is within allowed bounds. */
[[nodiscard]] util::Result<void> CheckBlockReservedWeight(size_t block_reserved_weight, const std::string& arg_name = "");

/** Check that coinbase_output_max_additional_sigops does not exceed consensus limits. */
[[nodiscard]] util::Result<void> CheckCoinbaseOutputMaxAdditionalSigops(size_t sigops, const std::string& arg_name = "");

} // namespace node

#endif // BITCOIN_NODE_MINING_ARGS_H
