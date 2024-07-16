// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_MINING_H
#define BITCOIN_INTERFACES_MINING_H

#include <node/types.h>
#include <primitives/block.h>
#include <util/time.h>

#include <memory>
#include <optional>
#include <uint256.h>

namespace node {
struct CBlockTemplate;
struct NodeContext;
} // namespace node

class BlockValidationState;
class CBlock;
class CScript;

namespace interfaces {

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

//! Interface giving clients (RPC, Stratum v2 Template Provider in the future)
//! ability to create block templates.

class Mining
{
public:
    virtual ~Mining() = default;

    //! If this chain is exclusively used for testing
    virtual bool isTestChain() = 0;

    //! Returns whether IBD is still in progress.
    virtual bool isInitialBlockDownload() = 0;

    //! Returns the hash for the tip of this chain
    virtual std::optional<uint256> getTipHash() = 0;

    // Implemented in https://github.com/bitcoin/bitcoin/pull/30409
    virtual std::optional<int> getTipHeight() { return {}; }
    virtual std::pair<uint256, int> waitTipChanged(MillisecondsDouble timeout = MillisecondsDouble::max()) { return {}; }
    // Implemented in https://github.com/bitcoin/bitcoin/pull/30443
    virtual bool waitFeesChanged(MillisecondsDouble timeout, uint256 tip, CAmount fee_delta = 0, CAmount fees_before = 0) { return {}; }

   /**
     * Construct a new block template
     *
     * @param[in] script_pub_key the coinbase output
     * @param[in] use_mempool set false to omit mempool transactions
     * @returns a block template
     */
    virtual std::unique_ptr<node::CBlockTemplate> createNewBlock(const CScript& script_pub_key, bool use_mempool = true) = 0;

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
