// Copyright (c) 2021-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//! @file
//! @brief Common init functions shared by bitcoin-node, bitcoin-wallet, etc.

#ifndef BITCOIN_INIT_COMMON_H
#define BITCOIN_INIT_COMMON_H

#include <util/result.h>

class ArgsManager;

namespace init {
void AddLoggingArgs(ArgsManager& args);
<<<<<<< HEAD
void SetLoggingOptions(const ArgsManager& args);
[[nodiscard]] util::Result<void> SetLoggingCategories(const ArgsManager& args);
[[nodiscard]] util::Result<void> SetLoggingLevel(const ArgsManager& args);
||||||| parent of 06380b771a00 (multiprocess: Add debug.log .wallet/.gui suffixes)
void SetLoggingOptions(const ArgsManager& args);
void SetLoggingCategories(const ArgsManager& args);
void SetLoggingLevel(const ArgsManager& args);
=======
void SetLoggingOptions(const ArgsManager& args, const char* log_suffix);
void SetLoggingCategories(const ArgsManager& args);
void SetLoggingLevel(const ArgsManager& args);
>>>>>>> 06380b771a00 (multiprocess: Add debug.log .wallet/.gui suffixes)
bool StartLogging(const ArgsManager& args);
void LogPackageVersion();
} // namespace init

#endif // BITCOIN_INIT_COMMON_H
