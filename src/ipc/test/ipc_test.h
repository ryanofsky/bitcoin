// Copyright (c) 2023-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_TEST_IPC_TEST_H
#define BITCOIN_IPC_TEST_IPC_TEST_H

#include <primitives/transaction.h>
#include <script/script.h>
#include <univalue.h>
#include <util/fs.h>
#include <validation.h>

#include <future>
#include <stop_token>

class FooImplementation
{
public:
    int add(int a, int b) { return a + b; }
    COutPoint passOutPoint(COutPoint o) { return o; }
    UniValue passUniValue(UniValue v) { return v; }
    CTransactionRef passTransaction(CTransactionRef t) { return t; }
    std::vector<CTransactionRef> passTransactions(std::vector<CTransactionRef> t) { return t; }
    std::vector<char> passVectorChar(std::vector<char> v) { return v; }
    CScript passScript(CScript s) { return s; }
    void initThreadMap() {}
    //! Wait until stop is requested on the token.
    void waitStop(std::stop_token stop_token)
    {
        std::promise<void> stopped;
        const std::stop_callback on_stop{stop_token, [&] { stopped.set_value(); }};
        stopped.get_future().wait();
    }
};

#endif // BITCOIN_IPC_TEST_IPC_TEST_H
