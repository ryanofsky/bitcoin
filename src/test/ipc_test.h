// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_IPC_TEST_H
#define BITCOIN_TEST_IPC_TEST_H

#include <primitives/transaction.h>
#include <univalue.h>
<<<<<<< HEAD
#include <util/fs.h>
||||||| parent of 5487cf806859 (multiprocess: Add serialization code for BlockValidationState)
=======
#include <validation.h>
>>>>>>> 5487cf806859 (multiprocess: Add serialization code for BlockValidationState)

class FooImplementation
{
public:
    int add(int a, int b) { return a + b; }
    COutPoint passOutPoint(COutPoint o) { return o; }
    UniValue passUniValue(UniValue v) { return v; }
    CTransactionRef passTransaction(CTransactionRef t) { return t; }
    BlockValidationState passBlockState(BlockValidationState s) { return s; }
    std::vector<char> passVectorChar(std::vector<char> v) { return v; }
};

void IpcPipeTest();
void IpcSocketPairTest();
void IpcSocketTest(const fs::path& datadir);

#endif // BITCOIN_TEST_IPC_TEST_H
