// Copyright (c) 2016-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/test/wallet_test_fixture.h>

WalletTestingSetup::WalletTestingSetup(const std::string& chainName)
    : TestingSetup(chainName),
      m_wallet(m_chain.get(), WalletLocation(), CreateMockWalletDatabase())
{
    bool fFirstRun;
    m_wallet.LoadWallet(fFirstRun);
    CWallet::AttachChain({ &m_wallet, [](CWallet*) {} });
    m_chain_client->registerRpcs();
}
