// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_CHAIN_H
#define BITCOIN_NODE_CHAIN_H

#include <functional>

class CBlockIndex;
class CChain;
class CThreadInterrupt;
namespace node {
class BlockManager;
} // namespace node

namespace node {
//! Send blockConnected and blockDisconnected notifications needed to sync from
//! a specified block to the chain tip.
//!
//! @param chain - chain to sync to
//! @param block - starting block to sync from
//! @param notifications - object to send notifications to
//! @param interrupt - flag to interrupt the sync
void SyncChain(BlockManager& blockman, const CChain& chain, const CBlockIndex* block, std::shared_ptr<interfaces::Chain::Notifications> notifications, const CThreadInterrupt& interrupt);
} // namespace node

#endif // BITCOIN_NODE_CHAIN_H
