// Copyright (c) 2020-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TEST_UTIL_VALIDATION_H
#define BITCOIN_TEST_UTIL_VALIDATION_H

#include <validation.h>

<<<<<<< HEAD
class CValidationInterface;

struct TestChainstateManager : public ChainstateManager {
||||||| parent of 20e68aefe8d6 (indexes, refactor: Remove index RegisterValidationInterface call)
class CValidationInterface;

struct TestChainState : public Chainstate {
=======
struct TestChainState : public Chainstate {
>>>>>>> 20e68aefe8d6 (indexes, refactor: Remove index RegisterValidationInterface call)
    /** Reset the ibd cache to its initial state */
    void ResetIbd();
    /** Toggle IsInitialBlockDownload from true to false */
    void JumpOutOfIbd();
};

#endif // BITCOIN_TEST_UTIL_VALIDATION_H
