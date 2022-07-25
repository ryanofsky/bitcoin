// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_DUMP_H
#define BITCOIN_WALLET_DUMP_H

#include <util/fs.h>
#include <util/result.h>

#include <string>

struct bilingual_str;
class ArgsManager;

namespace wallet {
class WalletDatabase;

<<<<<<< HEAD
bool DumpWallet(const ArgsManager& args, WalletDatabase& db, bilingual_str& error);
bool CreateFromDump(const ArgsManager& args, const std::string& name, const fs::path& wallet_path, bilingual_str& error);
||||||| parent of ea704a7d306 (refactor: Use util::Result class in wallet/dump)
bool DumpWallet(const ArgsManager& args, WalletDatabase& db, bilingual_str& error);
bool CreateFromDump(const ArgsManager& args, const std::string& name, const fs::path& wallet_path, bilingual_str& error, std::vector<bilingual_str>& warnings);
=======
util::Result<void> DumpWallet(const ArgsManager& args, WalletDatabase& db);
util::Result<void> CreateFromDump(const ArgsManager& args, const std::string& name, const fs::path& wallet_path);
>>>>>>> ea704a7d306 (refactor: Use util::Result class in wallet/dump)
} // namespace wallet

#endif // BITCOIN_WALLET_DUMP_H
