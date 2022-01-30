// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <interfaces/chain.h>
#include <kernel/chain.h>
#include <node/blockstorage.h>
#include <node/chain.h>
#include <sync.h>
#include <uint256.h>
#include <undo.h>
#include <util/threadinterrupt.h>

using interfaces::BlockInfo;
using kernel::MakeBlockInfo;

namespace node {
static const CBlockIndex* NextSyncBlock(const CBlockIndex* pindex_prev, const CChain& chain) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
{
    AssertLockHeld(cs_main);

    if (!pindex_prev) {
        return chain.Genesis();
    }

    const CBlockIndex* pindex = chain.Next(pindex_prev);
    if (pindex) {
        return pindex;
    }

    return chain.Next(chain.FindFork(pindex_prev));
}

void SyncChain(BlockManager& blockman, const CChain& chain, const CBlockIndex* block, std::shared_ptr<interfaces::Chain::Notifications> notifications, const CThreadInterrupt& interrupt)
{
    const CBlockIndex* pindex = block;

    while (true) {
        if (interrupt) {
            LogPrintf("%s: interrupt set; exiting sync\n");

            return;
        }

        {
            LOCK(cs_main);
            const CBlockIndex* pindex_next = NextSyncBlock(pindex, chain);
            if (!pindex_next) {
                assert(pindex);
                notifications->blockConnected(ChainstateRole::NORMAL, kernel::MakeBlockInfo(pindex));
                notifications->chainStateFlushed(ChainstateRole::NORMAL, ::GetLocator(pindex));
                break;
            }
            if (pindex_next->pprev != pindex) {
                const CBlockIndex* current_tip = pindex;
                const CBlockIndex* new_tip = pindex_next->pprev;
                for (const CBlockIndex* iter_tip = current_tip; iter_tip != new_tip; iter_tip = iter_tip->pprev) {
                    CBlock block;
                    interfaces::BlockInfo block_info = kernel::MakeBlockInfo(iter_tip);
                    block_info.chain_tip = false;
                    if (!blockman.ReadBlockFromDisk(block, *iter_tip)) {
                        block_info.error = strprintf("%s: Failed to read block %s from disk",
                                __func__, iter_tip->GetBlockHash().ToString());
                    } else {
                        block_info.data = &block;
                    }
                    notifications->blockDisconnected(block_info);
                    if (interrupt) break;
                }
            }
            pindex = pindex_next;
        }

        CBlock block;
        interfaces::BlockInfo block_info = kernel::MakeBlockInfo(pindex);
        block_info.chain_tip = false;
        if (!blockman.ReadBlockFromDisk(block, *pindex)) {
            block_info.error = strprintf("%s: Failed to read block %s from disk",
                    __func__, pindex->GetBlockHash().ToString());
        } else {
            block_info.data = &block;
        }
        notifications->blockConnected(ChainstateRole::NORMAL, block_info);
    }
}
} // namespace node
