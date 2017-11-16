// Copyright (c) 2016 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/test/wallet_test_fixture.h>

#include <rpc/server.h>
#include <wallet/db.h>
#include <wallet/wallet.h>

std::unique_ptr<CWallet> pwalletMain;

WalletTestingSetup::WalletTestingSetup(const std::string& chainName):
    TestingSetup(chainName)
{
    bool fFirstRun;
    pwalletMain = MakeUnique<CWallet>(CWallet::Mock());
    pwalletMain->LoadWallet(fFirstRun);
    RegisterValidationInterface(pwalletMain.get());

    RegisterWalletRPCCommands(tableRPC);
}

WalletTestingSetup::~WalletTestingSetup()
{
    UnregisterValidationInterface(pwalletMain.get());
    pwalletMain.reset();
}
