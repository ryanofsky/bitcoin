// Copyright (c) 2020-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_VALIDATION_H
#define BITCOIN_TEST_UTIL_VALIDATION_H

#include <validation.h>

<<<<<<< HEAD
namespace node {
class BlockManager;
}
class CValidationInterface;

struct TestBlockManager : public node::BlockManager {
    /** Test-only method to clear internal state for fuzzing */
    void CleanupForFuzzing();
};

||||||| parent of 4662905f3e0 (indexes, refactor: Remove index RegisterValidationInterface call)
class CValidationInterface;

=======
>>>>>>> 4662905f3e0 (indexes, refactor: Remove index RegisterValidationInterface call)
struct TestChainstateManager : public ChainstateManager {
    /** Disable the next write of all chainstates */
    void DisableNextWrite();
    /** Reset the ibd cache to its initial state */
    void ResetIbd();
    /** Toggle IsInitialBlockDownload from true to false */
    void JumpOutOfIbd();
    /** Wrappers that avoid making chainstatemanager internals public for tests */
    void InvalidBlockFound(CBlockIndex* pindex, const BlockValidationState& state) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void InvalidChainFound(CBlockIndex* pindexNew) EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    CBlockIndex* FindMostWorkChain() EXCLUSIVE_LOCKS_REQUIRED(cs_main);
    void ResetBestInvalid() EXCLUSIVE_LOCKS_REQUIRED(cs_main);
};

<<<<<<< HEAD
class ValidationInterfaceTest
{
public:
    static void BlockConnected(
        const kernel::ChainstateRole& role,
        CValidationInterface& obj,
        const std::shared_ptr<const CBlock>& block,
        const CBlockIndex* pindex);
};

||||||| parent of 4662905f3e0 (indexes, refactor: Remove index RegisterValidationInterface call)
class ValidationInterfaceTest
{
public:
    static void BlockConnected(
        ChainstateRole role,
        CValidationInterface& obj,
        const std::shared_ptr<const CBlock>& block,
        const CBlockIndex* pindex);
};

=======
>>>>>>> 4662905f3e0 (indexes, refactor: Remove index RegisterValidationInterface call)
#endif // BITCOIN_TEST_UTIL_VALIDATION_H
