// Copyright (c) 2022-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CHAINSTATEMANAGER_ARGS_H
#define BITCOIN_NODE_CHAINSTATEMANAGER_ARGS_H

#include <util/result.h>
#include <validation.h>

class ArgsManager;

<<<<<<< HEAD
/** -par default (number of script-checking threads, 0 = auto) */
inline constexpr int DEFAULT_SCRIPTCHECK_THREADS{0};

||||||| parent of 56a43d69ea8 (move-only: move node constants to settings header)
/** -par default (number of script-checking threads, 0 = auto) */
static constexpr int DEFAULT_SCRIPTCHECK_THREADS{0};

=======
>>>>>>> 56a43d69ea8 (move-only: move node constants to settings header)
namespace node {
[[nodiscard]] util::Result<void> ApplyArgsManOptions(const ArgsManager& args, ChainstateManager::Options& opts);
} // namespace node

#endif // BITCOIN_NODE_CHAINSTATEMANAGER_ARGS_H
