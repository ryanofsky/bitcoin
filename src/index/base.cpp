// Copyright (c) 2017-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <common/args.h>
#include <index/base.h>
#include <interfaces/chain.h>
#include <interfaces/handler.h>
#include <kernel/chain.h>
#include <logging.h>
#include <node/abort.h>
#include <node/blockstorage.h>
#include <node/context.h>
#include <node/database_args.h>
#include <node/interface_ui.h>
#include <tinyformat.h>
#include <undo.h>
#include <util/thread.h>
#include <util/translation.h>
#include <validation.h> // For g_chainman
#include <warnings.h>

#include <string>
#include <utility>

constexpr uint8_t DB_BEST_BLOCK{'B'};

constexpr auto SYNC_LOG_INTERVAL{30s};
constexpr auto SYNC_LOCATOR_WRITE_INTERVAL{30s};

template <typename... Args>
void BaseIndex::FatalErrorf(const char* fmt, const Args&... args)
{
    Interrupt(); // Cancel the sync thread
    auto message = tfm::format(fmt, args...);
    node::AbortNode(m_chain->context()->shutdown, m_chain->context()->exit_status, message);
}

const CBlockIndex& BaseIndex::BlockIndex(const uint256& hash)
{
   return WITH_LOCK(cs_main, return *Assert(m_chainstate->m_blockman.LookupBlockIndex(hash)));
}

CBlockLocator GetLocator(interfaces::Chain& chain, const uint256& block_hash)
{
    CBlockLocator locator;
    bool found = chain.findBlock(block_hash, interfaces::FoundBlock().locator(locator));
    assert(found);
    assert(!locator.IsNull());
    return locator;
}

class BaseIndexNotifications : public interfaces::Chain::Notifications
{
public:
    BaseIndexNotifications(BaseIndex& index) : m_index(index) {}
    void blockConnected(const interfaces::BlockInfo& block) override;
    void blockDisconnected(const interfaces::BlockInfo& block) override;
    void chainStateFlushed(const CBlockLocator& locator) override;
    BaseIndex& m_index;
    interfaces::Chain::NotifyOptions m_options = m_index.CustomOptions();
    std::chrono::steady_clock::time_point m_last_log_time{0s};
    std::chrono::steady_clock::time_point m_last_locator_write_time{0s};
    //! As blocks are disconnected, index is updated but not committed to until
    //! the next flush or block connection. m_rewind_start points to the first
    //! block that has been disconnected and not flushed yet. m_rewind_error
    //! is set if a block failed to disconnect.
    const CBlockIndex* m_rewind_start = nullptr;
    bool m_rewind_error = false;
};

void BaseIndexNotifications::blockConnected(const interfaces::BlockInfo& block_info)
{
    // Make a mutable copy of the BlockInfo argument so undo_data can be
    // attached below. This is temporary and removed in upcoming commits.
    interfaces::BlockInfo block{block_info};

    if (!block.error.empty()) {
        m_index.FatalErrorf("%s", block.error);
        return;
    }

    const CBlockIndex* pindex = &m_index.BlockIndex(block.hash);
    if (!block.data) {
        // Null block.data means block is the ending block at the end of a sync,
        // so just update the best block and m_synced.
        m_index.SetBestBlockIndex(pindex);
        if (block.chain_tip) {
            m_index.m_synced = true;
            CallFunctionInValidationInterfaceQueue([this] { m_index.m_ready = true; });
            if (pindex) {
                LogPrintf("%s is enabled at height %d\n", m_index.GetName(), pindex->nHeight);
            } else {
                LogPrintf("%s is enabled\n", m_index.GetName());
            }
        }
        return;
    }

    if (m_index.IgnoreBlockConnected(block)) return;

    // If blocks were disconnected, flush index state to disk before connecting new blocks.
    bool rewind_ok = !m_rewind_start || !m_rewind_error;
    if (m_rewind_start && rewind_ok) {
        const CBlockIndex* best_block_index = m_index.m_best_block_index.load();
        assert(!best_block_index || best_block_index->GetAncestor(pindex->nHeight - 1) == pindex->pprev);
        chainStateFlushed(GetLocator(*m_index.m_chain, pindex->pprev->GetBlockHash()));
        m_index.SetBestBlockIndex(pindex->pprev);
        rewind_ok = m_index.m_best_block_index == pindex->pprev;
    }

    if (!rewind_ok) {
        m_index.FatalErrorf("%s: Failed to rewind index %s to a previous chain tip",
                   __func__, m_index.GetName());
        return;
    }

    CBlockUndo block_undo;
    if (m_options.connect_undo_data && !block.undo_data && pindex->nHeight > 0) {
        if (!m_index.m_chainstate->m_blockman.UndoReadFromDisk(block_undo, *pindex)) {
            m_index.FatalErrorf("%s: Failed to read block %s undo data from disk",
                       __func__, pindex->GetBlockHash().ToString());
            return;
        }
        block.undo_data = &block_undo;
    }

    std::chrono::steady_clock::time_point current_time{0s};
    if (!block.chain_tip) {
        current_time = std::chrono::steady_clock::now();
        if (m_last_log_time + SYNC_LOG_INTERVAL < current_time) {
            LogPrintf("Syncing %s with block chain from height %d\n",
                      m_index.GetName(), pindex->nHeight);
            m_last_log_time = current_time;
        }
    }

    if (!m_index.CustomAppend(block)) {
        m_index.FatalErrorf("%s: Failed to write block %s to index",
                   __func__, pindex->GetBlockHash().ToString());
        return;
    }

    if (!block.chain_tip && (m_last_locator_write_time + SYNC_LOCATOR_WRITE_INTERVAL < current_time || WITH_LOCK(m_index.m_mutex, return !m_index.m_notifications.get()))) {
        auto locator = GetLocator(*m_index.m_chain, pindex->GetBlockHash());
        m_last_locator_write_time = current_time;
        // No need to handle errors in Commit. If it fails, the error will be already be
        // logged. The best way to recover is to continue, as index cannot be corrupted by
        // a missed commit to disk for an advanced index state.
        m_index.Commit(locator);
    } else if (!block.chain_tip) {
        // Only update index best block between flushes if fully synced.
        // Decision to let the best block pointer lag during sync seems a
        // little arbitrary, but has been behavior since syncing was introduced
        // in #13033, so preserving it in case anything depends on it.
        return;
    }

    // Setting the best block index is intentionally the last step of this
    // function, so BlockUntilSyncedToCurrentChain callers waiting for the
    // best block index to be updated can rely on the block being fully
    // processed, and the index object being safe to delete.
    m_index.SetBestBlockIndex(pindex);
}

void BaseIndexNotifications::blockDisconnected(const interfaces::BlockInfo& block_info)
{
    // Make a mutable copy of the BlockInfo argument so block data can be
    // attached below. This is temporary and removed in upcoming commits.
    interfaces::BlockInfo block{block_info};

    if (!block.error.empty()) {
        m_index.FatalErrorf("%s", block.error);
        return;
    }

    // During initial sync, ignore validation interface notifications, only
    // process notifications from sync thread.
    if (!m_index.m_ready && block.chain_tip) return;

    const CBlockIndex* pindex = &m_index.BlockIndex(block.hash);
    if (!m_rewind_start) m_rewind_start = pindex;
    if (m_rewind_error) return;

    CBlockUndo block_undo;
    if (m_options.disconnect_undo_data && !block.undo_data && block.height > 0) {
        if (!m_index.m_chainstate->m_blockman.UndoReadFromDisk(block_undo, *pindex)) {
            // If undo data can't be read, subsequent CustomRemove calls will be
            // skipped, and will be a fatal error if there an attempt to connect
            // a another block to the index.
            m_rewind_error = true;
            return;
        }
        block.undo_data = &block_undo;
    }

    m_rewind_error = m_rewind_error || !m_index.CustomRemove(block);
}

void BaseIndexNotifications::chainStateFlushed(const CBlockLocator& locator)
{
    if (m_index.IgnoreChainStateFlushed(locator)) return;

    // No need to handle errors in Commit. If it fails, the error will be already be logged. The
    // best way to recover is to continue, as index cannot be corrupted by a missed commit to disk
    // for an advanced index state.
    // In the case of a reorg, ensure persisted block locator is not stale.
    // Pruning has a minimum of 288 blocks-to-keep and getting the index
    // out of sync may be possible but a users fault.
    // In case we reorg beyond the pruned depth, ReadBlockFromDisk would
    // throw and lead to a graceful shutdown
    if (!m_index.Commit(locator) && m_rewind_start) {
        // If commit fails, revert the best block index to avoid corruption.
        m_index.SetBestBlockIndex(m_rewind_start);
    }
    m_rewind_start = nullptr;
    m_rewind_error = false;
}

BaseIndex::DB::DB(const fs::path& path, size_t n_cache_size, bool f_memory, bool f_wipe, bool f_obfuscate) :
    CDBWrapper{DBParams{
        .path = path,
        .cache_bytes = n_cache_size,
        .memory_only = f_memory,
        .wipe_data = f_wipe,
        .obfuscate = f_obfuscate,
        .options = [] { DBOptions options; node::ReadDatabaseArgs(gArgs, options); return options; }()}}
{}

bool BaseIndex::DB::ReadBestBlock(CBlockLocator& locator) const
{
    bool success = Read(DB_BEST_BLOCK, locator);
    if (!success) {
        locator.SetNull();
    }
    return success;
}

void BaseIndex::DB::WriteBestBlock(CDBBatch& batch, const CBlockLocator& locator)
{
    batch.Write(DB_BEST_BLOCK, locator);
}

BaseIndex::BaseIndex(std::unique_ptr<interfaces::Chain> chain, std::string name)
    : m_chain{std::move(chain)}, m_name{std::move(name)} {}

BaseIndex::~BaseIndex()
{
    //! Assert Stop() was called before this base destructor. Notification
    //! handlers call pure virtual methods like GetName(), so if they are still
    //! being called at this point, they would segfault.
    LOCK(m_mutex);
    assert(!m_notifications);
    assert(!m_handler);
}

// Read index best block, register for block connected and disconnected
// notifications, and determine where best block is relative to chain tip.
//
// If the chain tip and index best block are the same, block connected and
// disconnected notifications will be enabled after this call and the index will
// update as the ImportBlocks() function connects blocks and sends
// notifications. Otherwise, when the chain tip and index best block not the
// same, the index will stay idle until ImportBlocks() finishes and
// BaseIndex::StartBackgroundSync() is called after.
//
// If the node is being started for the first time, or if -reindex or
// -reindex-chainstate are used, the chain tip will be null at this point,
// meaning that no blocks are attached, even a genesis block. The best block
// locator will also be null if -reindex is used or if the index is new, but
// will be non-null if -reindex-chainstate is used. So -reindex will cause the
// index to be considered synced and rebuild right away as the chain is
// rebuilt, while -reindex-chainstate will cause the index to be idle until the
// chain is rebuilt and BaseIndex::StartBackgroundSync is called after.
//
// All of this just ensures that -reindex and -reindex-chainstate options both
// function efficiently. If -reindex is used, both the chainstate and index are
// wiped, and the index is considered synced right away and gets rebuilt at the
// same time as the chainstate. If -reindex-chainstate is used, only the
// chainstate is wiped, not the index, so the index will be considered not
// synced, and the chainstate will update first, and the index will start
// syncing after. So the most efficient thing should happen in both cases.
bool BaseIndex::Init()
{
    AssertLockNotHeld(cs_main);

    // May need reset if index is being restarted.
    m_interrupt.reset();

    // m_chainstate member gives indexing code access to node internals. It is
    // removed in followup https://github.com/bitcoin/bitcoin/pull/24230
<<<<<<< HEAD
    m_chainstate = WITH_LOCK(::cs_main,
        return &m_chain->context()->chainman->GetChainstateForIndexing());
    // Register to validation interface before setting the 'm_synced' flag, so that
    // callbacks are not missed once m_synced is true.
    m_chain->context()->validation_signals->RegisterValidationInterface(this);
||||||| parent of 28dfda309f51 (indexes: Avoid race, make -reindex-chainstate more efficient)
    m_chainstate = &m_chain->context()->chainman->ActiveChainstate();
    // Register to validation interface before setting the 'm_synced' flag, so that
    // callbacks are not missed once m_synced is true.
    RegisterValidationInterface(this);
=======
    m_chainstate = &m_chain->context()->chainman->ActiveChainstate();

<<<<<<< HEAD
    // Register to receive validation interface notifications. These
    // notifications will be ignored until m_ready is set to true, so there is
    // no harm in registering too early. Registering any time before cs_main is
    // released at the end of this function would be early enough to avoid
    // missing notifications.
    RegisterValidationInterface(this);
>>>>>>> 28dfda309f51 (indexes: Avoid race, make -reindex-chainstate more efficient)

||||||| parent of 1a79ce5d35e0 (indexes, refactor: Remove index RegisterValidationInterface call)
    // Register to receive validation interface notifications. These
    // notifications will be ignored until m_ready is set to true, so there is
    // no harm in registering too early. Registering any time before cs_main is
    // released at the end of this function would be early enough to avoid
    // missing notifications.
    RegisterValidationInterface(this);

=======
>>>>>>> 1a79ce5d35e0 (indexes, refactor: Remove index RegisterValidationInterface call)
    CBlockLocator locator;
    if (!GetDB().ReadBestBlock(locator)) {
        locator.SetNull();
    }

<<<<<<< HEAD
    LOCK(cs_main);
    CChain& index_chain = m_chainstate->m_chain;

    if (locator.IsNull()) {
        SetBestBlockIndex(nullptr);
    } else {
        // Setting the best block to the locator's top block. If it is not part of the
        // best chain, we will rewind to the fork point during index sync
        const CBlockIndex* locator_index{m_chainstate->m_blockman.LookupBlockIndex(locator.vHave.at(0))};
        if (!locator_index) {
            return InitError(strprintf(Untranslated("%s: best block of the index not found. Please rebuild the index."), GetName()));
||||||| parent of 1a79ce5d35e0 (indexes, refactor: Remove index RegisterValidationInterface call)
    LOCK(cs_main);
    CChain& active_chain = m_chainstate->m_chain;
    if (locator.IsNull()) {
        SetBestBlockIndex(nullptr);
    } else {
        // Setting the best block to the locator's top block. If it is not part of the
        // best chain, we will rewind to the fork point during index sync
        const CBlockIndex* locator_index{m_chainstate->m_blockman.LookupBlockIndex(locator.vHave.at(0))};
        if (!locator_index) {
            return InitError(strprintf(Untranslated("%s: best block of the index not found. Please rebuild the index."), GetName()));
=======
    auto options = CustomOptions();
    options.thread_name = GetName();
    auto notifications = std::make_shared<BaseIndexNotifications>(*this);
    auto start_sync = [&](const interfaces::BlockInfo& block) {
        const auto block_key{block.height >= 0 ? std::make_optional(interfaces::BlockKey{block.hash, block.height}) : std::nullopt};
        if (!locator.IsNull() && !block_key) {
            return InitError(strprintf(Untranslated("%s: best block of the index not found. Please rebuild the index, or disable it until the node is synced."), GetName()));
>>>>>>> 1a79ce5d35e0 (indexes, refactor: Remove index RegisterValidationInterface call)
        }

        assert(!m_best_block_index && !m_synced);
        SetBestBlockIndex(block_key ? &BlockIndex(block_key->hash) : nullptr);

<<<<<<< HEAD
    // Note: this will latch to true immediately if the user starts up with an empty
    // datadir and an index enabled. If this is the case, indexation will happen solely
    // via `BlockConnected` signals until, possibly, the next restart.
<<<<<<< HEAD
    m_synced = start_block == index_chain.Tip();
||||||| parent of 28dfda309f51 (indexes: Avoid race, make -reindex-chainstate more efficient)
    m_synced = start_block == active_chain.Tip();
=======
    m_synced = start_block == active_chain.Tip();
    if (m_synced) {
||||||| parent of 1a79ce5d35e0 (indexes, refactor: Remove index RegisterValidationInterface call)
    // Note: this will latch to true immediately if the user starts up with an empty
    // datadir and an index enabled. If this is the case, indexation will happen solely
    // via `BlockConnected` signals until, possibly, the next restart.
    m_synced = start_block == active_chain.Tip();
    if (m_synced) {
=======
        // Call CustomInit and set m_ready. It is important to call CustomInit
        // before setting m_ready to ensure that CustomInit is always called
        // before CustomAppend. CustomAppend calls from the notification thread
        // will start happening when m_ready is true.
        if (!CustomInit(block_key)) {
            return false;
        }
>>>>>>> 1a79ce5d35e0 (indexes, refactor: Remove index RegisterValidationInterface call)
        // To prevent race conditions, m_ready = true needs to be set from the
        // validationinterface thread and the m_ready = true callback needs to
        // be queued while cs_main is held.
        //
        // To prevent older, stale notifications currently in the validation
        // queue from being processed by the index, it is important to delay
        // setting m_ready = true until they are removed from the queue.
        //
        // To prevent new notifications that may be happening in the background
        // right now from being lost, it is important to keep cs_main locked
        // while calling CallFunctionInValidationInterfaceQueue, so the new
        // notifications will be queued after the m_ready = true callback.
<<<<<<< HEAD
        CallFunctionInValidationInterfaceQueue([this] { m_ready = true; });
    }
>>>>>>> 28dfda309f51 (indexes: Avoid race, make -reindex-chainstate more efficient)
    m_init = true;
||||||| parent of 1a79ce5d35e0 (indexes, refactor: Remove index RegisterValidationInterface call)
        CallFunctionInValidationInterfaceQueue([this] { m_ready = true; });
    }
    m_init = true;
=======
        m_synced = block.chain_tip;
        if (m_synced) {
            CallFunctionInValidationInterfaceQueue([this] { m_ready = true; });
        }
        return true;
    };
    auto handler = m_chain->attachChain(notifications, locator, options, start_sync);

    // Handler will be null if start_sync lambda above returned false.
    if (!handler) return false;

    LOCK(m_mutex);
    m_notifications = std::move(notifications);
    m_handler = std::move(handler);
>>>>>>> 1a79ce5d35e0 (indexes, refactor: Remove index RegisterValidationInterface call)
    return true;
}

<<<<<<< HEAD
static const CBlockIndex* NextSyncBlock(const CBlockIndex* pindex_prev, CChain& chain) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
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

void BaseIndex::ThreadSync()
{
    const CBlockIndex* pindex = m_best_block_index.load();
    if (!m_synced) {
        auto notifications = WITH_LOCK(m_mutex, return m_notifications);

        while (true) {
            if (m_interrupt) {
<<<<<<< HEAD
                LogPrintf("%s: m_interrupt set; exiting ThreadSync\n", GetName());

                SetBestBlockIndex(pindex);
                // No need to handle errors in Commit. If it fails, the error will be already be
                // logged. The best way to recover is to continue, as index cannot be corrupted by
                // a missed commit to disk for an advanced index state.
                Commit(GetLocator(*m_chain, pindex->GetBlockHash()));
||||||| parent of 5a8918a833a2 (indexes, refactor: Move more new block logic out of ThreadSync to blockConnected)
                SetBestBlockIndex(pindex);
                // No need to handle errors in Commit. If it fails, the error will be already be
                // logged. The best way to recover is to continue, as index cannot be corrupted by
                // a missed commit to disk for an advanced index state.
                Commit(GetLocator(*m_chain, pindex->GetBlockHash()));
=======
>>>>>>> 5a8918a833a2 (indexes, refactor: Move more new block logic out of ThreadSync to blockConnected)
                return;
            }

            {
                LOCK(cs_main);
                const CBlockIndex* pindex_next = NextSyncBlock(pindex, m_chainstate->m_chain);
                if (!pindex_next) {
                    assert(pindex);
                    notifications->blockConnected(kernel::MakeBlockInfo(pindex));
                    notifications->chainStateFlushed(GetLocator(*m_chain, pindex->GetBlockHash()));
                    break;
                }
                if (pindex_next->pprev != pindex) {
                    const CBlockIndex* current_tip = pindex;
                    const CBlockIndex* new_tip = pindex_next->pprev;
                    for (const CBlockIndex* iter_tip = current_tip; iter_tip != new_tip; iter_tip = iter_tip->pprev) {
                        CBlock block;
                        interfaces::BlockInfo block_info = kernel::MakeBlockInfo(iter_tip);
                        block_info.chain_tip = false;
                        notifications->blockDisconnected(block_info);
                        if (m_interrupt) break;
                    }
                }
                pindex = pindex_next;
            }

            CBlock block;
            interfaces::BlockInfo block_info = kernel::MakeBlockInfo(pindex);
            block_info.chain_tip = false;
            if (!m_chainstate->m_blockman.ReadBlockFromDisk(block, *pindex)) {
                block_info.error = strprintf("%s: Failed to read block %s from disk",
                           __func__, pindex->GetBlockHash().ToString());
            } else {
                block_info.data = &block;
            }
            notifications->blockConnected(block_info);
        }
    }
}

||||||| parent of c60c73532069 (indexes, refactor: Move sync thread from index to node)
static const CBlockIndex* NextSyncBlock(const CBlockIndex* pindex_prev, CChain& chain) EXCLUSIVE_LOCKS_REQUIRED(cs_main)
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

void BaseIndex::ThreadSync()
{
    const CBlockIndex* pindex = m_best_block_index.load();
    if (!m_synced) {
        auto notifications = WITH_LOCK(m_mutex, return m_notifications);

        while (true) {
            if (m_interrupt) {
                return;
            }

            {
                LOCK(cs_main);
                const CBlockIndex* pindex_next = NextSyncBlock(pindex, m_chainstate->m_chain);
                if (!pindex_next) {
                    assert(pindex);
                    notifications->blockConnected(kernel::MakeBlockInfo(pindex));
                    notifications->chainStateFlushed(GetLocator(*m_chain, pindex->GetBlockHash()));
                    break;
                }
                if (pindex_next->pprev != pindex) {
                    const CBlockIndex* current_tip = pindex;
                    const CBlockIndex* new_tip = pindex_next->pprev;
                    for (const CBlockIndex* iter_tip = current_tip; iter_tip != new_tip; iter_tip = iter_tip->pprev) {
                        CBlock block;
                        interfaces::BlockInfo block_info = kernel::MakeBlockInfo(iter_tip);
                        block_info.chain_tip = false;
                        notifications->blockDisconnected(block_info);
                        if (m_interrupt) break;
                    }
                }
                pindex = pindex_next;
            }

            CBlock block;
            interfaces::BlockInfo block_info = kernel::MakeBlockInfo(pindex);
            block_info.chain_tip = false;
            if (!m_chainstate->m_blockman.ReadBlockFromDisk(block, *pindex)) {
                block_info.error = strprintf("%s: Failed to read block %s from disk",
                           __func__, pindex->GetBlockHash().ToString());
            } else {
                block_info.data = &block;
            }
            notifications->blockConnected(block_info);
        }
    }
}

=======
>>>>>>> c60c73532069 (indexes, refactor: Move sync thread from index to node)
bool BaseIndex::Commit(const CBlockLocator& locator)
{
    // Don't commit anything if we haven't indexed any block yet
    // (this could happen if init is interrupted).
    bool ok = !locator.IsNull();
    if (ok) {
        CDBBatch batch(GetDB());
        ok = CustomCommit(batch);
        if (ok) {
            GetDB().WriteBestBlock(batch, locator);
            ok = GetDB().WriteBatch(batch);
        }
    }
    if (!ok) {
        LogError("%s: Failed to commit latest %s state\n", __func__, GetName());
        return false;
    }
    return true;
}

<<<<<<< HEAD
bool BaseIndex::Rewind(const CBlockIndex* current_tip, const CBlockIndex* new_tip)
{
    assert(current_tip == m_best_block_index);
    assert(current_tip->GetAncestor(new_tip->nHeight) == new_tip);

    CBlock block;
    CBlockUndo block_undo;

    for (const CBlockIndex* iter_tip = current_tip; iter_tip != new_tip; iter_tip = iter_tip->pprev) {
        interfaces::BlockInfo block_info = kernel::MakeBlockInfo(iter_tip);
        if (CustomOptions().disconnect_data) {
            if (!m_chainstate->m_blockman.ReadBlockFromDisk(block, *iter_tip)) {
                return error("%s: Failed to read block %s from disk",
                             __func__, iter_tip->GetBlockHash().ToString());
            }
            block_info.data = &block;
        }
        if (CustomOptions().disconnect_undo_data && iter_tip->nHeight > 0) {
            if (!m_chainstate->m_blockman.UndoReadFromDisk(block_undo, *iter_tip)) {
                return false;
            }
            block_info.undo_data = &block_undo;
        }
        if (!CustomRemove(block_info)) {
            return false;
        }
    }

    // In the case of a reorg, ensure persisted block locator is not stale.
    // Pruning has a minimum of 288 blocks-to-keep and getting the index
    // out of sync may be possible but a users fault.
    // In case we reorg beyond the pruned depth, ReadBlockFromDisk would
    // throw and lead to a graceful shutdown
    SetBestBlockIndex(new_tip);
    if (!Commit(GetLocator(*m_chain, new_tip->GetBlockHash()))) {
        // If commit fails, revert the best block index to avoid corruption.
        SetBestBlockIndex(current_tip);
        return false;
    }

    return true;
}

<<<<<<< HEAD
<<<<<<< HEAD
void BaseIndex::BlockConnected(ChainstateRole role, const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex)
||||||| parent of 1a79ce5d35e0 (indexes, refactor: Remove index RegisterValidationInterface call)
void BaseIndex::BlockConnected(const std::shared_ptr<const CBlock>& block, const CBlockIndex* pindex)
=======
void BaseIndex::BlockConnected(const interfaces::BlockInfo& block_info)
>>>>>>> 1a79ce5d35e0 (indexes, refactor: Remove index RegisterValidationInterface call)
||||||| parent of 8d0cc07587d4 (indexes, refactor: Remove index validationinterface hooks)
void BaseIndex::BlockConnected(const interfaces::BlockInfo& block_info)
=======
||||||| parent of 8a164cf087c4 (indexes, refactor: Move Rewind logic out of Rewind to blockDisconnected and ThreadSync)
bool BaseIndex::Rewind(const CBlockIndex* current_tip, const CBlockIndex* new_tip)
{
    assert(current_tip == m_best_block_index);
    assert(current_tip->GetAncestor(new_tip->nHeight) == new_tip);

    CBlock block;
    CBlockUndo block_undo;

    for (const CBlockIndex* iter_tip = current_tip; iter_tip != new_tip; iter_tip = iter_tip->pprev) {
        interfaces::BlockInfo block_info = kernel::MakeBlockInfo(iter_tip);
        if (CustomOptions().disconnect_data) {
            if (!m_chainstate->m_blockman.ReadBlockFromDisk(block, *iter_tip)) {
                return error("%s: Failed to read block %s from disk",
                             __func__, iter_tip->GetBlockHash().ToString());
            }
            block_info.data = &block;
        }
        if (CustomOptions().disconnect_undo_data && iter_tip->nHeight > 0) {
            if (!m_chainstate->m_blockman.UndoReadFromDisk(block_undo, *iter_tip)) {
                return false;
            }
            block_info.undo_data = &block_undo;
        }
        if (!CustomRemove(block_info)) {
            return false;
        }
    }

    // In the case of a reorg, ensure persisted block locator is not stale.
    // Pruning has a minimum of 288 blocks-to-keep and getting the index
    // out of sync may be possible but a users fault.
    // In case we reorg beyond the pruned depth, ReadBlockFromDisk would
    // throw and lead to a graceful shutdown
    SetBestBlockIndex(new_tip);
    if (!Commit(GetLocator(*m_chain, new_tip->GetBlockHash()))) {
        // If commit fails, revert the best block index to avoid corruption.
        SetBestBlockIndex(current_tip);
        return false;
    }

    return true;
}

=======
>>>>>>> 8a164cf087c4 (indexes, refactor: Move Rewind logic out of Rewind to blockDisconnected and ThreadSync)
bool BaseIndex::IgnoreBlockConnected(const interfaces::BlockInfo& block)
>>>>>>> 8d0cc07587d4 (indexes, refactor: Remove index validationinterface hooks)
{
<<<<<<< HEAD
<<<<<<< HEAD
    // Ignore events from the assumed-valid chain; we will process its blocks
    // (sequentially) after it is fully verified by the background chainstate. This
    // is to avoid any out-of-order indexing.
    //
    // TODO at some point we could parameterize whether a particular index can be
    // built out of order, but for now just do the conservative simple thing.
    if (role == ChainstateRole::ASSUMEDVALID) {
        return;
    }

    // Ignore BlockConnected signals until we have fully indexed the chain.
    if (!m_synced) {
||||||| parent of 28dfda309f51 (indexes: Avoid race, make -reindex-chainstate more efficient)
    if (!m_synced) {
=======
||||||| parent of f2a0551eb8ec (indexes, refactor: Remove remaining CBlockIndex* uses in index CustomAppend methods)
=======
    // During initial sync, ignore validation interface notifications, only
    // process notifications from sync thread.
>>>>>>> f2a0551eb8ec (indexes, refactor: Remove remaining CBlockIndex* uses in index CustomAppend methods)
    if (!m_ready) {
<<<<<<< HEAD
<<<<<<< HEAD
>>>>>>> 28dfda309f51 (indexes: Avoid race, make -reindex-chainstate more efficient)
        return;
||||||| parent of 8d0cc07587d4 (indexes, refactor: Remove index validationinterface hooks)
        return;
=======
        return true;
>>>>>>> 8d0cc07587d4 (indexes, refactor: Remove index validationinterface hooks)
||||||| parent of f2a0551eb8ec (indexes, refactor: Remove remaining CBlockIndex* uses in index CustomAppend methods)
        return true;
=======
        return block.chain_tip;
>>>>>>> f2a0551eb8ec (indexes, refactor: Remove remaining CBlockIndex* uses in index CustomAppend methods)
    }

    const CBlockIndex* pindex = &BlockIndex(block.hash);
    const CBlockIndex* best_block_index = m_best_block_index.load();
    if (!best_block_index) {
        if (pindex->nHeight != 0) {
            FatalErrorf("%s: First block connected is not the genesis block (height=%d)",
                       __func__, pindex->nHeight);
            return true;
        }
    } else {
        // To allow handling reorgs, this only checks that the new block
        // connects to ancestor of the current best block, instead of checking
        // that it connects to directly to the current block. If there is a
        // reorg, blockDisconnected calls will have removed existing blocks from
        // the index, but best_block_index will have be updated yet.
        assert(best_block_index->GetAncestor(pindex->nHeight - 1) == pindex->pprev);
    }
    return false;
}

<<<<<<< HEAD
void BaseIndex::ChainStateFlushed(ChainstateRole role, const CBlockLocator& locator)
||||||| parent of 8d0cc07587d4 (indexes, refactor: Remove index validationinterface hooks)
void BaseIndex::ChainStateFlushed(const CBlockLocator& locator)
=======
bool BaseIndex::IgnoreChainStateFlushed(const CBlockLocator& locator)
>>>>>>> 8d0cc07587d4 (indexes, refactor: Remove index validationinterface hooks)
{
<<<<<<< HEAD
<<<<<<< HEAD
    // Ignore events from the assumed-valid chain; we will process its blocks
    // (sequentially) after it is fully verified by the background chainstate.
    if (role == ChainstateRole::ASSUMEDVALID) {
        return;
    }

    if (!m_synced) {
||||||| parent of 28dfda309f51 (indexes: Avoid race, make -reindex-chainstate more efficient)
    if (!m_synced) {
=======
    if (!m_ready) {
<<<<<<< HEAD
>>>>>>> 28dfda309f51 (indexes: Avoid race, make -reindex-chainstate more efficient)
        return;
||||||| parent of 8d0cc07587d4 (indexes, refactor: Remove index validationinterface hooks)
        return;
=======
        return true;
>>>>>>> 8d0cc07587d4 (indexes, refactor: Remove index validationinterface hooks)
    }

||||||| parent of 8a164cf087c4 (indexes, refactor: Move Rewind logic out of Rewind to blockDisconnected and ThreadSync)
    if (!m_ready) {
        return true;
    }

=======
    assert(!locator.IsNull());
>>>>>>> 8a164cf087c4 (indexes, refactor: Move Rewind logic out of Rewind to blockDisconnected and ThreadSync)
    const uint256& locator_tip_hash = locator.vHave.front();
    const CBlockIndex* locator_tip_index;
    {
        LOCK(cs_main);
        locator_tip_index = m_chainstate->m_blockman.LookupBlockIndex(locator_tip_hash);
    }

    if (!locator_tip_index) {
        FatalErrorf("%s: First block (hash=%s) in locator was not found",
                   __func__, locator_tip_hash.ToString());
        return true;
    }

    // Assert locator points to the last block that was connected, or ancestor
    // of it. (It may point to ancestor block if the last block was invalidated,
    // or if a reorg started and there was a BlockDisconnected notification
    // between the last BlockConnected notification and ChainstateFlushed.)
    const CBlockIndex* best_block_index = m_best_block_index.load();
    if (best_block_index->GetAncestor(locator_tip_index->nHeight) != locator_tip_index) {
        assert(!m_ready);
        return true;
    }
    return false;
}

bool BaseIndex::BlockUntilSyncedToCurrentChain() const
{
    AssertLockNotHeld(cs_main);

    if (!m_synced) {
        return false;
    }

    {
        // Skip the queue-draining stuff if we know we're caught up with
        // m_chain.Tip().
        LOCK(cs_main);
        const CBlockIndex* chain_tip = m_chainstate->m_chain.Tip();
        const CBlockIndex* best_block_index = m_best_block_index.load();
        if (best_block_index->GetAncestor(chain_tip->nHeight) == chain_tip) {
            return true;
        }
    }

    LogPrintf("%s: %s is catching up on block notifications\n", __func__, GetName());
    m_chain->context()->validation_signals->SyncWithValidationInterfaceQueue();
    return true;
}

void BaseIndex::Interrupt()
{
    LOCK(m_mutex);
    if (m_handler) m_handler->interrupt();
    m_notifications.reset();
}

bool BaseIndex::StartBackgroundSync()
{
    LOCK(m_mutex);
    if (!m_handler) throw std::logic_error("Error: Cannot start a non-initialized index");
    m_handler->start();
    return true;
}

void BaseIndex::Stop()
{
<<<<<<< HEAD
<<<<<<< HEAD
<<<<<<< HEAD
    if (m_chain->context()->validation_signals) {
        m_chain->context()->validation_signals->UnregisterValidationInterface(this);
    }
||||||| parent of 1a79ce5d35e0 (indexes, refactor: Remove index RegisterValidationInterface call)
    UnregisterValidationInterface(this);
=======
    WITH_LOCK(m_mutex, m_handler.reset());
>>>>>>> 1a79ce5d35e0 (indexes, refactor: Remove index RegisterValidationInterface call)
||||||| parent of f2a0551eb8ec (indexes, refactor: Remove remaining CBlockIndex* uses in index CustomAppend methods)
    WITH_LOCK(m_mutex, m_handler.reset());
=======
    {
        m_interrupt();
        LOCK(m_mutex);
        m_notifications.reset();
        m_handler.reset();
    }
>>>>>>> f2a0551eb8ec (indexes, refactor: Remove remaining CBlockIndex* uses in index CustomAppend methods)

    if (m_thread_sync.joinable()) {
        m_thread_sync.join();
    }
||||||| parent of c60c73532069 (indexes, refactor: Move sync thread from index to node)
    {
        m_interrupt();
        LOCK(m_mutex);
        m_notifications.reset();
        m_handler.reset();
    }

    if (m_thread_sync.joinable()) {
        m_thread_sync.join();
    }
=======
    Interrupt();
    // Call handler destructor after releasing m_mutex. Locking the mutex is
    // required to access m_handler, but the lock should not be held while
    // destroying the handler, because the handler destructor waits for the last
    // notification to be processed, so holding the lock would deadlock if that
    // last notification also needs the lock.
    auto handler = WITH_LOCK(m_mutex, return std::move(m_handler));
>>>>>>> c60c73532069 (indexes, refactor: Move sync thread from index to node)
}

IndexSummary BaseIndex::GetSummary() const
{
    IndexSummary summary{};
    summary.name = GetName();
    summary.synced = m_synced;
    if (const auto& pindex = m_best_block_index.load()) {
        summary.best_block_height = pindex->nHeight;
        summary.best_block_hash = pindex->GetBlockHash();
    } else {
        summary.best_block_height = 0;
        summary.best_block_hash = m_chain->getBlockHash(0);
    }
    return summary;
}

void BaseIndex::SetBestBlockIndex(const CBlockIndex* block)
{
    assert(!m_chainstate->m_blockman.IsPruneMode() || AllowPrune());

    if (AllowPrune() && block) {
        node::PruneLockInfo prune_lock;
        prune_lock.height_first = block->nHeight;
        WITH_LOCK(::cs_main, m_chainstate->m_blockman.UpdatePruneLock(GetName(), prune_lock));
    }

    // Intentionally set m_best_block_index as the last step in this function,
    // after updating prune locks above, and after making any other references
    // to *this, so the BlockUntilSyncedToCurrentChain function (which checks
    // m_best_block_index as an optimization) can be used to wait for the last
    // BlockConnected notification and safely assume that prune locks are
    // updated and that the index object is safe to delete.
    m_best_block_index = block;
}
