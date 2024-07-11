// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_MINING_H
#define BITCOIN_INTERFACES_MINING_H

<<<<<<< HEAD
#include <consensus/amount.h>       // for CAmount
#include <interfaces/types.h>       // for BlockRef
#include <node/types.h>             // for BlockCreateOptions
#include <primitives/block.h>       // for CBlock, CBlockHeader
#include <primitives/transaction.h> // for CTransactionRef
#include <stdint.h>                 // for int64_t
#include <uint256.h>                // for uint256
#include <util/time.h>              // for MillisecondsDouble

#include <memory>   // for unique_ptr, shared_ptr
#include <optional> // for optional
#include <vector>   // for vector
||||||| parent of 4e1a4342f3b2 (multiprocess: Expand mining interface)
#include <memory>
#include <optional>
#include <uint256.h>
=======
#include <node/types.h>
#include <primitives/block.h>
#include <util/time.h>

#include <memory>
#include <optional>
#include <uint256.h>
>>>>>>> 4e1a4342f3b2 (multiprocess: Expand mining interface)

namespace common {
struct Settings;
} // namespace common
namespace node {
struct NodeContext;
} // namespace node

class BlockValidationState;
class CScript;

namespace interfaces {

<<<<<<< HEAD
//! Block template interface
class BlockTemplate
{
public:
    virtual ~BlockTemplate() = default;

    virtual CBlockHeader getBlockHeader() = 0;
    virtual CBlock getBlock() = 0;

    virtual std::vector<CAmount> getTxFees() = 0;
    virtual std::vector<int64_t> getTxSigops() = 0;

    virtual CTransactionRef getCoinbaseTx() = 0;
    virtual std::vector<unsigned char> getCoinbaseCommitment() = 0;
    virtual int getWitnessCommitmentIndex() = 0;
};

||||||| parent of 4e1a4342f3b2 (multiprocess: Expand mining interface)
=======
// Implemented in https://github.com/bitcoin/bitcoin/pull/30440
class BlockTemplate
{
public:
    virtual ~BlockTemplate() = default;
    virtual CBlockHeader getBlockHeader() { return {}; }
    virtual CBlock getBlock() { return {}; }
    virtual std::vector<CAmount> getTxFees() { return {}; }
    virtual std::vector<int64_t> getTxSigops() { return {}; }
    virtual CTransactionRef getCoinbaseTx() { return {}; }
    virtual std::vector<unsigned char> getCoinbaseCommitment() { return {}; }
    virtual int getWitnessCommitmentIndex() { return {}; }
    virtual std::vector<uint256> getCoinbaseMerklePath() { return {}; }
    virtual bool submitSolution(uint32_t version, uint32_t timestamp, uint32_t nonce, CMutableTransaction coinbase) { return {}; }
};

>>>>>>> 4e1a4342f3b2 (multiprocess: Expand mining interface)
//! Interface giving clients (RPC, Stratum v2 Template Provider in the future)
//! ability to create block templates.
class Mining
{
public:
    virtual ~Mining() = default;

    //! Start node. Return false if node failed to start up or was already started.
    virtual bool startNode(const common::Settings& settings, int argc, const char* const argv[]) = 0;

    // Stop node. Return false if node was not started.
    virtual bool stopNode(int& exit_status) = 0;

    //! If this chain is exclusively used for testing
    virtual bool isTestChain() = 0;

    //! Returns whether IBD is still in progress.
    virtual bool isInitialBlockDownload() = 0;

    //! Returns the hash and height for the tip of this chain
    virtual std::optional<BlockRef> getTip() = 0;

    /**
     * Waits for the tip to change
     *
     * @param[in] current_tip block hash of the current chain tip. Function waits
     *                        for the chain tip to change if this matches, otherwise
     *                        it returns right away.
     * @param[in] timeout     how long to wait for a new tip
     * @returns               Hash and height of the current chain tip after this call.
     */
    virtual BlockRef waitTipChanged(uint256 current_tip, MillisecondsDouble timeout = MillisecondsDouble::max()) = 0;

    // Implemented in https://github.com/bitcoin/bitcoin/pull/30409
    virtual std::optional<int> getTipHeight() { return {}; }
    virtual std::pair<uint256, int> waitTipChanged(MillisecondsDouble timeout = MillisecondsDouble::max()) { return {}; }
    // Implemented in https://github.com/bitcoin/bitcoin/pull/30443
    virtual bool waitFeesChanged(MillisecondsDouble timeout, uint256 tip, CAmount fee_delta = 0, CAmount fees_before = 0) { return {}; }

   /**
     * Construct a new block template
     *
     * @param[in] script_pub_key the coinbase output
     * @param[in] options options for creating the block
     * @returns a block template
     */
    virtual std::unique_ptr<BlockTemplate> createNewBlock(const CScript& script_pub_key, const node::BlockCreateOptions& options = {}) = 0;

    // Implemented in https://github.com/bitcoin/bitcoin/pull/30356
    virtual std::unique_ptr<BlockTemplate> createNewBlock2(const CScript& script_pub_key, const node::BlockCreateOptions& options={}) { return {}; }

    /**
     * Processes new block. A valid new block is automatically relayed to peers.
     *
     * @param[in]   block The block we want to process.
     * @param[out]  new_block A boolean which is set to indicate if the block was first received via this call
     * @returns     If the block was processed, independently of block validity
     */
    virtual bool processNewBlock(const std::shared_ptr<const CBlock>& block, bool* new_block) = 0;

    //! Return the number of transaction updates in the mempool,
    //! used to decide whether to make a new block template.
    virtual unsigned int getTransactionsUpdated() = 0;

    /**
     * Check a block is completely valid from start to finish.
     * Only works on top of our current best block.
     * Does not check proof-of-work.
     *
     * @param[in] block the block to validate
     * @param[in] check_merkle_root call CheckMerkleRoot()
     * @param[out] state details of why a block failed to validate
     * @returns false if it does not build on the current tip, or any of the checks fail
     */
    virtual bool testBlockValidity(const CBlock& block, bool check_merkle_root, BlockValidationState& state) = 0;

    //! Get internal node context. Useful for RPC and testing,
    //! but not accessible across processes.
    virtual node::NodeContext* context() { return nullptr; }
};

//! Return implementation of Mining interface.
std::unique_ptr<Mining> MakeMining(node::NodeContext& node);

} // namespace interfaces

#endif // BITCOIN_INTERFACES_MINING_H
