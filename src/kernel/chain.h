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
    //! Whether this is an event from the most-work (active) chainstate.
    bool most_work;

    //! Whether this is an event from chainstate that's been fully validated
    //! starting from the genesis block. False if is from an assumeutxo snapshot
    //! chainstate that has not been validated yet.
    bool validated;
};

std::ostream& operator<<(std::ostream& os, const ChainstateRole& role);

#endif // BITCOIN_KERNEL_CHAIN_H
