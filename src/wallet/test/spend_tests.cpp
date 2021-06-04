// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/fees.h>
#include <validation.h>
#include <wallet/coincontrol.h>
#include <wallet/test/util.h>
#include <wallet/test/wallet_test_fixture.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(spend_tests, WalletTestingSetup)

BOOST_FIXTURE_TEST_CASE(SubtractFee, TestChain100Setup)
{
    CChain& cchain = m_node.chainman->ActiveChain();
    CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    auto wallet = CreateSyncedWallet(*m_node.chain, cchain, coinbaseKey);

    // Check subtract from recipient transaction equal to coinbase amount does
    // not create change output.
    CRecipient recipient{GetScriptForRawPubKey({}), 50 * COIN, true /* subtract fee */};
    CTransactionRef tx;
    CAmount fee;
    int change_pos = -1;
    bilingual_str error;
    CCoinControl coin_control;
    FeeCalculation fee_calc;
    BOOST_CHECK(wallet->CreateTransaction({recipient}, tx, fee, change_pos, error, coin_control, fee_calc));
    BOOST_CHECK_EQUAL(tx->vout.size(), 1);
    BOOST_CHECK_EQUAL(tx->vout[0].nValue, recipient.nAmount - fee);
    tx.reset();

    // Check subtract from recipient transaction slightly less than coinbase
    // amount also does not create change output and pays extra dust amount to
    // recipient instead of miner
    const CAmount dust_amount = 123;
    const CAmount expected_fee = fee;
    recipient.nAmount -= dust_amount;
    BOOST_CHECK(wallet->CreateTransaction({recipient}, tx, fee, change_pos, error, coin_control, fee_calc));
    BOOST_CHECK_EQUAL(tx->vout.size(), 1);
    BOOST_CHECK_EQUAL(tx->vout[0].nValue, recipient.nAmount - fee + dust_amount);
    BOOST_CHECK_EQUAL(fee, expected_fee);
    tx.reset();
}

BOOST_AUTO_TEST_SUITE_END()
