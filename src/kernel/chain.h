// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KERNEL_CHAIN_H
#define BITCOIN_KERNEL_CHAIN_H

<<<<<<< HEAD
#include<iostream>

||||||| parent of 1664a6357894 (indexes: Rewrite chain sync logic, remove racy init)
=======
#include <interfaces/chain.h>

#include <functional>

>>>>>>> 1664a6357894 (indexes: Rewrite chain sync logic, remove racy init)
class CBlock;
class CBlockIndex;
class CChain;
class CThreadInterrupt;
namespace node {
class BlockManager;
} // namespace node

namespace kernel {
//! Return data from block index.
interfaces::BlockInfo MakeBlockInfo(const CBlockIndex* block_index, const CBlock* data = nullptr);
<<<<<<< HEAD

||||||| parent of 1664a6357894 (indexes: Rewrite chain sync logic, remove racy init)
=======

//! Send blockConnected and blockDisconnected notifications needed to sync from
//! a specified block to the chain tip. This sync function locks the ::cs_main
//! mutex intermittently, releasing it while sending notifications and reading
//! block data, so it follows the tip if the tip changes, and follows any
//! reorgs.
//!
//! @param chain - chain to sync to
//! @param block - starting block to sync from
//! @param notifications - object to send notifications to
//! @param interrupt - flag to interrupt the sync
//! @param on_sync - optional callback invoked when reaching the chain tip
//!                  while cs_main is still held, before sending a final
//!                  blockConnected notification. This can be used to
//!                  synchronously register for new notifications.
//! @return true if synced, false if interrupted
bool SyncChain(node::BlockManager& blockman, const CChain& chain, const CBlockIndex* block, std::shared_ptr<interfaces::Chain::Notifications> notifications, const CThreadInterrupt& interrupt, std::function<void()> on_sync);
>>>>>>> 1664a6357894 (indexes: Rewrite chain sync logic, remove racy init)
} // namespace kernel

//! This enum describes the various roles a specific Chainstate instance can take.
//! Other parts of the system sometimes need to vary in behavior depending on the
//! existence of a background validation chainstate, e.g. when building indexes.
enum class ChainstateRole {
    // Single chainstate in use, "normal" IBD mode.
    NORMAL,

    // Doing IBD-style validation in the background. Implies use of an assumed-valid
    // chainstate.
    BACKGROUND,

    // Active assumed-valid chainstate. Implies use of a background IBD chainstate.
    ASSUMEDVALID,
};

std::ostream& operator<<(std::ostream& os, const ChainstateRole& role);

#endif // BITCOIN_KERNEL_CHAIN_H
