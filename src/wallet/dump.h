// Copyright (c) 2020-2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_DUMP_H
#define BITCOIN_WALLET_DUMP_H

#include <util/fs.h>
#include <util/result.h>

#include <string>
#include <vector>

struct bilingual_str;
class ArgsManager;

namespace wallet {
<<<<<<< HEAD
class WalletDatabase;

bool DumpWallet(const ArgsManager& args, WalletDatabase& db, bilingual_str& error);
bool CreateFromDump(const ArgsManager& args, const std::string& name, const fs::path& wallet_path, bilingual_str& error, std::vector<bilingual_str>& warnings);
||||||| parent of 1b2a5f12b425 (refactor: Use util::Result class for wallet loading)
class CWallet;
bool DumpWallet(const ArgsManager& args, CWallet& wallet, bilingual_str& error);
bool CreateFromDump(const ArgsManager& args, const std::string& name, const fs::path& wallet_path, bilingual_str& error, std::vector<bilingual_str>& warnings);
=======
class CWallet;
util::Result<void> DumpWallet(const ArgsManager& args, CWallet& wallet);
util::Result<void> CreateFromDump(const ArgsManager& args, const std::string& name, const fs::path& wallet_path);
>>>>>>> 1b2a5f12b425 (refactor: Use util::Result class for wallet loading)
} // namespace wallet

#endif // BITCOIN_WALLET_DUMP_H
