// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <node/caches.h>

#include <common/args.h>
#include <index/txindex.h>
<<<<<<< HEAD
#include <kernel/caches.h>
#include <logging.h>
#include <util/byte_units.h>

#include <algorithm>
#include <string>

// Unlike for the UTXO database, for the txindex scenario the leveldb cache make
// a meaningful difference: https://github.com/bitcoin/bitcoin/pull/8273#issuecomment-229601991
//! Max memory allocated to tx index DB specific cache in bytes.
static constexpr size_t MAX_TX_INDEX_CACHE{1024_MiB};
//! Max memory allocated to all block filter index caches combined in bytes.
static constexpr size_t MAX_FILTER_INDEX_CACHE{1024_MiB};
||||||| parent of e12edf766138 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
#include <txdb.h>
=======
#include <init_settings.h>
#include <txdb.h>
>>>>>>> e12edf766138 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)

namespace node {
CacheSizes CalculateCacheSizes(const ArgsManager& args, size_t n_indexes)
{
<<<<<<< HEAD
    // Convert -dbcache from MiB units to bytes. The total cache is floored by MIN_DB_CACHE and capped by max size_t value.
    size_t total_cache{DEFAULT_DB_CACHE};
    if (std::optional<int64_t> db_cache = args.GetIntArg("-dbcache")) {
        if (*db_cache < 0) db_cache = 0;
        uint64_t db_cache_bytes = SaturatingLeftShift<uint64_t>(*db_cache, 20);
        total_cache = std::max<size_t>(MIN_DB_CACHE, std::min<uint64_t>(db_cache_bytes, std::numeric_limits<size_t>::max()));
||||||| parent of e12edf766138 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
    int64_t nTotalCache = (args.GetIntArg("-dbcache", nDefaultDbCache) << 20);
    nTotalCache = std::max(nTotalCache, nMinDbCache << 20); // total cache cannot be less than nMinDbCache
    CacheSizes sizes;
    sizes.block_tree_db = std::min(nTotalCache / 8, nMaxBlockDBCache << 20);
    nTotalCache -= sizes.block_tree_db;
    sizes.tx_index = std::min(nTotalCache / 8, args.GetBoolArg("-txindex", DEFAULT_TXINDEX) ? nMaxTxIndexCache << 20 : 0);
    nTotalCache -= sizes.tx_index;
    sizes.filter_index = 0;
    if (n_indexes > 0) {
        int64_t max_cache = std::min(nTotalCache / 8, max_filter_index_cache << 20);
        sizes.filter_index = max_cache / n_indexes;
        nTotalCache -= sizes.filter_index * n_indexes;
=======
    int64_t nTotalCache = (DbCacheSetting::Get(args) << 20);
    nTotalCache = std::max(nTotalCache, nMinDbCache << 20); // total cache cannot be less than nMinDbCache
    CacheSizes sizes;
    sizes.block_tree_db = std::min(nTotalCache / 8, nMaxBlockDBCache << 20);
    nTotalCache -= sizes.block_tree_db;
    sizes.tx_index = std::min(nTotalCache / 8, TxIndexSetting::Get(args) ? nMaxTxIndexCache << 20 : 0);
    nTotalCache -= sizes.tx_index;
    sizes.filter_index = 0;
    if (n_indexes > 0) {
        int64_t max_cache = std::min(nTotalCache / 8, max_filter_index_cache << 20);
        sizes.filter_index = max_cache / n_indexes;
        nTotalCache -= sizes.filter_index * n_indexes;
>>>>>>> e12edf766138 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
    }

    IndexCacheSizes index_sizes;
    index_sizes.tx_index = std::min(total_cache / 8, args.GetBoolArg("-txindex", DEFAULT_TXINDEX) ? MAX_TX_INDEX_CACHE : 0);
    total_cache -= index_sizes.tx_index;
    if (n_indexes > 0) {
        size_t max_cache = std::min(total_cache / 8, MAX_FILTER_INDEX_CACHE);
        index_sizes.filter_index = max_cache / n_indexes;
        total_cache -= index_sizes.filter_index * n_indexes;
    }
    return {index_sizes, kernel::CacheSizes{total_cache}};
}
} // namespace node
