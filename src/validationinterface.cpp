// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validationinterface.h>

#include <chain.h>
#include <consensus/validation.h>
#include <logging.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <scheduler.h>

#include <future>
#include <unordered_map>
#include <utility>

struct MainSignalsInstance {
    Mutex m_mutex;
    struct Entry { std::shared_ptr<CValidationInterface> callbacks; int count = 1; };
    std::list<Entry> m_list GUARDED_BY(m_mutex);
    std::unordered_map<CValidationInterface*, std::list<Entry>::iterator> m_map GUARDED_BY(m_mutex);
    // We are not allowed to assume the scheduler only runs in one thread,
    // but must ensure all callbacks happen in-order, so we end up creating
    // our own queue here :(
    SingleThreadedSchedulerClient m_schedulerClient;

    explicit MainSignalsInstance(CScheduler *pscheduler) : m_schedulerClient(pscheduler) {}

    void Add(std::shared_ptr<CValidationInterface> callbacks)
    {
        LOCK(m_mutex);
        auto inserted = m_map.emplace(callbacks.get(), m_list.end());
        if (inserted.second) {
            m_list.emplace_back();
            inserted.first->second = std::prev(m_list.end());
        }
        inserted.first->second->callbacks = std::move(callbacks);
    }

    void Remove(CValidationInterface* key)
    {
        LOCK(m_mutex);
        auto it = m_map.find(key);
        if (it != m_map.end()) {
            if (--it->second->count == 0) m_list.erase(it->second);
            m_map.erase(it);
        }
    }

    void Clear()
    {
        LOCK(m_mutex);
        for (auto it = m_list.begin(); it != m_list.end();) {
            if (--it->count == 0) it = m_list.erase(it); else ++it;
        }
        m_map.clear();
    }

    template<typename F> void Iterate(F&& f)
    {
        WAIT_LOCK(m_mutex, lock);
        for (auto it(m_list.begin()), prev(m_list.end());; prev = it++) {
            if (prev != m_list.end() && --prev->count == 0) m_list.erase(prev);
            if (it == m_list.end()) break;
            ++it->count;
            REVERSE_LOCK(lock);
            f(*it->callbacks);
        }
    }
};

static CMainSignals g_signals;

void CMainSignals::RegisterBackgroundSignalScheduler(CScheduler& scheduler) {
    assert(!m_internals);
    m_internals.reset(new MainSignalsInstance(&scheduler));
}

void CMainSignals::UnregisterBackgroundSignalScheduler() {
    m_internals.reset(nullptr);
}

void CMainSignals::FlushBackgroundCallbacks() {
    if (m_internals) {
        m_internals->m_schedulerClient.EmptyQueue();
    }
}

size_t CMainSignals::CallbacksPending() {
    if (!m_internals) return 0;
    return m_internals->m_schedulerClient.CallbacksPending();
}

CMainSignals& GetMainSignals()
{
    return g_signals;
}

void RegisterSharedValidationInterface(std::shared_ptr<CValidationInterface> pwalletIn) {
    // Each connection captures pwalletIn to ensure that each callback is
    // executed before pwalletIn is destroyed. For more details see #18338.
    g_signals.m_internals->Add(std::move(pwalletIn));
}

void RegisterValidationInterface(CValidationInterface* callbacks)
{
    // Create a shared_ptr with a no-op deleter - CValidationInterface lifecycle
    // is managed by the caller.
    RegisterSharedValidationInterface({callbacks, [](CValidationInterface*){}});
}

void UnregisterSharedValidationInterface(std::shared_ptr<CValidationInterface> callbacks)
{
    UnregisterValidationInterface(callbacks.get());
}

void UnregisterValidationInterface(CValidationInterface* pwalletIn) {
    if (g_signals.m_internals) {
        g_signals.m_internals->Remove(pwalletIn);
    }
}

void UnregisterAllValidationInterfaces() {
    if (!g_signals.m_internals) {
        return;
    }
    g_signals.m_internals->Clear();
}

void CallFunctionInValidationInterfaceQueue(std::function<void ()> func) {
    g_signals.m_internals->m_schedulerClient.AddToProcessQueue(std::move(func));
}

void SyncWithValidationInterfaceQueue() {
    AssertLockNotHeld(cs_main);
    // Block until the validation queue drains
    std::promise<void> promise;
    CallFunctionInValidationInterfaceQueue([&promise] {
        promise.set_value();
    });
    promise.get_future().wait();
}

// Use a macro instead of a function for conditional logging to prevent
// evaluating arguments when logging is not enabled.
//
// NOTE: The lambda captures all local variables by value.
#define ENQUEUE_AND_LOG_EVENT(event, fmt, name, ...)           \
    do {                                                       \
        auto local_name = (name);                              \
        LOG_EVENT("Enqueuing " fmt, local_name, __VA_ARGS__);  \
        m_internals->m_schedulerClient.AddToProcessQueue([=] { \
            LOG_EVENT(fmt, local_name, __VA_ARGS__);           \
            event();                                           \
        });                                                    \
    } while (0)

#define LOG_EVENT(fmt, ...) \
    LogPrint(BCLog::VALIDATION, fmt "\n", __VA_ARGS__)

void CMainSignals::UpdatedBlockTip(const CBlockIndex *pindexNew, const CBlockIndex *pindexFork, bool fInitialDownload) {
    // Dependencies exist that require UpdatedBlockTip events to be delivered in the order in which
    // the chain actually updates. One way to ensure this is for the caller to invoke this signal
    // in the same critical section where the chain is updated

    auto event = [pindexNew, pindexFork, fInitialDownload, this] {
        m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.UpdatedBlockTip(pindexNew, pindexFork, fInitialDownload); });
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: new block hash=%s fork block hash=%s (in IBD=%s)", __func__,
                          pindexNew->GetBlockHash().ToString(),
                          pindexFork ? pindexFork->GetBlockHash().ToString() : "null",
                          fInitialDownload);
}

void CMainSignals::TransactionAddedToMempool(const CTransactionRef &ptx) {
    auto event = [ptx, this] {
        m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.TransactionAddedToMempool(ptx); });
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: txid=%s wtxid=%s", __func__,
                          ptx->GetHash().ToString(),
                          ptx->GetWitnessHash().ToString());
}

void CMainSignals::TransactionRemovedFromMempool(const CTransactionRef &ptx) {
    auto event = [ptx, this] {
        m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.TransactionRemovedFromMempool(ptx); });
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: txid=%s wtxid=%s", __func__,
                          ptx->GetHash().ToString(),
                          ptx->GetWitnessHash().ToString());
}

void CMainSignals::BlockConnected(const std::shared_ptr<const CBlock> &pblock, const CBlockIndex *pindex) {
    auto event = [pblock, pindex, this] {
        m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.BlockConnected(pblock, pindex); });
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: block hash=%s block height=%d", __func__,
                          pblock->GetHash().ToString(),
                          pindex->nHeight);
}

void CMainSignals::BlockDisconnected(const std::shared_ptr<const CBlock>& pblock, const CBlockIndex* pindex)
{
    auto event = [pblock, pindex, this] {
        m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.BlockDisconnected(pblock, pindex); });
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: block hash=%s block height=%d", __func__,
                          pblock->GetHash().ToString(),
                          pindex->nHeight);
}

void CMainSignals::ChainStateFlushed(const CBlockLocator &locator) {
    auto event = [locator, this] {
        m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.ChainStateFlushed(locator); });
    };
    ENQUEUE_AND_LOG_EVENT(event, "%s: block hash=%s", __func__,
                          locator.IsNull() ? "null" : locator.vHave.front().ToString());
}

void CMainSignals::BlockChecked(const CBlock& block, const BlockValidationState& state) {
    LOG_EVENT("%s: block hash=%s state=%s", __func__,
              block.GetHash().ToString(), state.ToString());
    m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.BlockChecked(block, state); });
}

void CMainSignals::NewPoWValidBlock(const CBlockIndex *pindex, const std::shared_ptr<const CBlock> &block) {
    LOG_EVENT("%s: block hash=%s", __func__, block->GetHash().ToString());
    m_internals->Iterate([&](CValidationInterface& callbacks) { callbacks.NewPoWValidBlock(pindex, block); });
}
