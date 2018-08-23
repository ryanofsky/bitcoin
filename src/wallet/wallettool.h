// Copyright (c) 2016-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_WALLETTOOL_H
#define BITCOIN_WALLET_WALLETTOOL_H

#include <wallet/wallet.h>

namespace interfaces {
class Chain;
} // namespace interfaces

namespace WalletTool {

<<<<<<< HEAD
||||||| merged common ancestors
std::shared_ptr<CWallet> CreateWallet(const std::string& name, const fs::path& path);
std::shared_ptr<CWallet> LoadWallet(const std::string& name, const fs::path& path);
=======
std::shared_ptr<CWallet> CreateWallet(interfaces::Chain* chain, const std::string& name, const fs::path& path);
std::shared_ptr<CWallet> LoadWallet(interfaces::Chain* chain, const std::string& name, const fs::path& path);
>>>>>>> multiprocess: Add -ipcconnect and -ipcbind options
void WalletShowInfo(CWallet* wallet_instance);
bool ExecuteWalletToolFunc(interfaces::Chain* chain, const std::string& command, const std::string& file);

} // namespace WalletTool

#endif // BITCOIN_WALLET_WALLETTOOL_H
