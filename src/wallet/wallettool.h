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

void WalletShowInfo(CWallet* wallet_instance);
<<<<<<< HEAD
bool ExecuteWalletToolFunc(const ArgsManager& args, const std::string& command, const std::string& file);
||||||| merged common ancestors
bool ExecuteWalletToolFunc(const std::string& command, const std::string& file);
=======
bool ExecuteWalletToolFunc(interfaces::Chain* chain, const std::string& command, const std::string& file);
>>>>>>> multiprocess: Add -ipcconnect and -ipcbind options

} // namespace WalletTool

#endif // BITCOIN_WALLET_WALLETTOOL_H
