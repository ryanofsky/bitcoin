// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_IPC_TEST_H
#define BITCOIN_TEST_IPC_TEST_H

#include <node/miner.h>
#include <primitives/transaction.h>
#include <univalue.h>
<<<<<<< HEAD
#include <util/fs.h>
#include <validation.h>
||||||| parent of b697050bb6e7 (multiprocess: Add unit tests for connect, serve, and listen functions)
=======
#include <util/fs.h>
>>>>>>> b697050bb6e7 (multiprocess: Add unit tests for connect, serve, and listen functions)

class FooImplementation
{
public:
    int add(int a, int b) { return a + b; }
    COutPoint passOutPoint(COutPoint o) { return o; }
    UniValue passUniValue(UniValue v) { return v; }
<<<<<<< HEAD
    CTransactionRef passTransaction(CTransactionRef t) { return t; }
    BlockValidationState passBlockState(BlockValidationState s) { return s; }
    std::vector<char> passVectorChar(std::vector<char> v) { return v; }
    node::CBlockTemplate passBlockTemplate(node::CBlockTemplate t) { return t; }
||||||| parent of d7a784df9449 (multiprocess: Add serialization code for CTransaction)
=======
    CTransactionRef passTransaction(CTransactionRef t) { return t; }
>>>>>>> d7a784df9449 (multiprocess: Add serialization code for CTransaction)
};

void IpcPipeTest();
void IpcSocketPairTest();
void IpcSocketTest(const fs::path& datadir);

#endif // BITCOIN_TEST_IPC_TEST_H
