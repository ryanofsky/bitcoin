// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_CHAIN_H
#define BITCOIN_KERNEL_CHAIN_H

#include<iostream>

class CBlock;
class CBlockIndex;
namespace interfaces {
struct BlockInfo;
} // namespace interfaces

namespace kernel {
//! Return data from block index.
interfaces::BlockInfo MakeBlockInfo(const CBlockIndex* block_index, const CBlock* data = nullptr);

} // namespace kernel

struct ChainstateRole {
    //! Whether this is an event from the chainstate syncing to the most-work
    //! block, as opposed a chainstate downloading historic blocks and being
    //! used to validate an assumeutxo snapshot.
    bool most_work{true};

    //! Whether this is an event from chainstate that's been fully validated
    //! starting from the genesis block. False if is from an assumeutxo snapshot
    //! chainstate that has not been validated yet.
    bool validated{true};
};

#endif // BITCOIN_KERNEL_CHAIN_H
