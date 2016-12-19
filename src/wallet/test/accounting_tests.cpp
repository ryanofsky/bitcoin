// Copyright (c) 2012-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/wallet.h>

#include <wallet/test/wallet_test_fixture.h>

#include <stdint.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(accounting_tests, WalletTestingSetup)

static void
GetResults(CWallet *wallet, std::map<CAmount, CAccountingEntry>& results)
{
    std::list<CAccountingEntry> aes;

    results.clear();
    BOOST_CHECK(wallet->ReorderTransactions() == DB_LOAD_OK);
    wallet->ListAccountCreditDebit("", aes);
    for (CAccountingEntry& ae : aes)
    {
        results[ae.nOrderPos] = ae;
    }
}

BOOST_AUTO_TEST_CASE(acc_orderupgrade)
{
    std::vector<CWalletTx*> vpwtx;
    CAccountingEntry ae;
    std::map<CAmount, CAccountingEntry> results;

    LOCK(pwalletMain->cs_wallet);

    ae.strAccount = "";
    ae.nCreditDebit = 1;
    ae.nTime = 1333333333;
    ae.strOtherAccount = "b";
    ae.strComment = "";
    pwalletMain->AddAccountingEntry(ae);

    CTransactionRef tx_new = MakeTransactionRef();
    pwalletMain->AddToWallet(tx_new, [&](CWalletTx& wtx, bool new_tx) {
        wtx.mapValue["comment"] = "z";
        vpwtx.push_back(&wtx);
        return true;
    });
    vpwtx[0]->nTimeReceived = (unsigned int)1333333335;
    vpwtx[0]->nOrderPos = -1;

    ae.nTime = 1333333336;
    ae.strOtherAccount = "c";
    pwalletMain->AddAccountingEntry(ae);

    GetResults(pwalletMain.get(), results);

    BOOST_CHECK(pwalletMain->nOrderPosNext == 3);
    BOOST_CHECK(2 == results.size());
    BOOST_CHECK(results[0].nTime == 1333333333);
    BOOST_CHECK(results[0].strComment.empty());
    BOOST_CHECK(1 == vpwtx[0]->nOrderPos);
    BOOST_CHECK(results[2].nTime == 1333333336);
    BOOST_CHECK(results[2].strOtherAccount == "c");


    ae.nTime = 1333333330;
    ae.strOtherAccount = "d";
    ae.nOrderPos = pwalletMain->IncOrderPosNext();
    pwalletMain->AddAccountingEntry(ae);

    GetResults(pwalletMain.get(), results);

    BOOST_CHECK(results.size() == 3);
    BOOST_CHECK(pwalletMain->nOrderPosNext == 4);
    BOOST_CHECK(results[0].nTime == 1333333333);
    BOOST_CHECK(1 == vpwtx[0]->nOrderPos);
    BOOST_CHECK(results[2].nTime == 1333333336);
    BOOST_CHECK(results[3].nTime == 1333333330);
    BOOST_CHECK(results[3].strComment.empty());


    {
        CMutableTransaction tx(*tx_new);
        --tx.nLockTime;  // Just to change the hash :)
        tx_new = MakeTransactionRef(std::move(tx));
    }
    pwalletMain->AddToWallet(tx_new, [&](CWalletTx& wtx, bool new_tx) {
        wtx.mapValue["comment"] = "y";
        vpwtx.push_back(&wtx);
        return true;
    });
    vpwtx[1]->nTimeReceived = (unsigned int)1333333336;

    {
        CMutableTransaction tx(*tx_new);
        --tx.nLockTime;  // Just to change the hash :)
        tx_new = MakeTransactionRef(std::move(tx));
    }
    pwalletMain->AddToWallet(tx_new, [&](CWalletTx& wtx, bool new_tx) {
        wtx.mapValue["comment"] = "x";
        vpwtx.push_back(&wtx);
        return true;
    });
    vpwtx[2]->nTimeReceived = (unsigned int)1333333329;
    vpwtx[2]->nOrderPos = -1;

    GetResults(pwalletMain.get(), results);

    BOOST_CHECK(results.size() == 3);
    BOOST_CHECK(pwalletMain->nOrderPosNext == 6);
    BOOST_CHECK(0 == vpwtx[2]->nOrderPos);
    BOOST_CHECK(results[1].nTime == 1333333333);
    BOOST_CHECK(2 == vpwtx[0]->nOrderPos);
    BOOST_CHECK(results[3].nTime == 1333333336);
    BOOST_CHECK(results[4].nTime == 1333333330);
    BOOST_CHECK(results[4].strComment.empty());
    BOOST_CHECK(5 == vpwtx[1]->nOrderPos);


    ae.nTime = 1333333334;
    ae.strOtherAccount = "e";
    ae.nOrderPos = -1;
    pwalletMain->AddAccountingEntry(ae);

    GetResults(pwalletMain.get(), results);

    BOOST_CHECK(results.size() == 4);
    BOOST_CHECK(pwalletMain->nOrderPosNext == 7);
    BOOST_CHECK(0 == vpwtx[2]->nOrderPos);
    BOOST_CHECK(results[1].nTime == 1333333333);
    BOOST_CHECK(2 == vpwtx[0]->nOrderPos);
    BOOST_CHECK(results[3].nTime == 1333333336);
    BOOST_CHECK(results[3].strComment.empty());
    BOOST_CHECK(results[4].nTime == 1333333330);
    BOOST_CHECK(results[4].strComment.empty());
    BOOST_CHECK(results[5].nTime == 1333333334);
    BOOST_CHECK(6 == vpwtx[1]->nOrderPos);
}

BOOST_AUTO_TEST_SUITE_END()
