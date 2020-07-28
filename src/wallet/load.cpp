// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/load.h>

#include <interfaces/chain.h>
#include <interfaces/wallet.h>
#include <scheduler.h>
#include <util/string.h>
#include <util/system.h>
#include <util/translation.h>
#include <wallet/wallet.h>
#include <wallet/walletdb.h>

namespace {
//! Shared pointer delete helper that does extra cleanup after deletion.
template<typename T>
struct Deleter
{
    void operator()(T* p)
    {
        delete p;
        if (cleanup) cleanup();
    }

    std::function<void()> cleanup;
};

//! Block waiting for wallet to finish loading or unloading. Blocking instead of
//! returning more complicated "in-progress" errors avoids need for simple RPC
//! clients to poll repeatedly when they want to load or unload wallets on
//! demand.
void WaitReady(WalletContext& context, UniqueLock<Mutex>& lock, const std::string& name)
{
    context.wallets_cv.wait(lock, [&]() EXCLUSIVE_LOCKS_REQUIRED(context.wallets_mutex) {
        auto it = context.wallets.find(name);
        return it == context.wallets.end() || it->second;
    });
}

//! Release shared pointers associated with wallet.
bool ReleaseWallet(std::shared_ptr<CWallet>&& wallet)
{
    if (!wallet) return false;
    wallet->m_chain_notifications_handler.reset();
    wallet->NotifyUnload();
    return true;
}
} // namespace

bool VerifyWallets(interfaces::Chain& chain, const std::vector<std::string>& wallet_files)
{
    if (gArgs.IsArgSet("-walletdir")) {
        fs::path wallet_dir = gArgs.GetArg("-walletdir", "");
        boost::system::error_code error;
        // The canonical path cleans the path, preventing >1 Berkeley environment instances for the same directory
        fs::path canonical_wallet_dir = fs::canonical(wallet_dir, error);
        if (error || !fs::exists(wallet_dir)) {
            chain.initError(strprintf(_("Specified -walletdir \"%s\" does not exist"), wallet_dir.string()));
            return false;
        } else if (!fs::is_directory(wallet_dir)) {
            chain.initError(strprintf(_("Specified -walletdir \"%s\" is not a directory"), wallet_dir.string()));
            return false;
        // The canonical path transforms relative paths into absolute ones, so we check the non-canonical version
        } else if (!wallet_dir.is_absolute()) {
            chain.initError(strprintf(_("Specified -walletdir \"%s\" is a relative path"), wallet_dir.string()));
            return false;
        }
        gArgs.ForceSetArg("-walletdir", canonical_wallet_dir.string());
    }

    LogPrintf("Using wallet directory %s\n", GetWalletDir().string());

    chain.initMessage(_("Verifying wallet(s)...").translated);

    // Keep track of each wallet absolute path to detect duplicates.
    std::set<fs::path> wallet_paths;

    for (const auto& wallet_file : wallet_files) {
        fs::path path;
        bilingual_str error_string;
        if (!CWallet::Path(wallet_file, path, error_string)) {
            chain.initError(error_string);
            return false;
        }

        if (!wallet_paths.insert(path).second) {
            chain.initError(strprintf(_("Error loading wallet %s. Duplicate -wallet filename specified."), wallet_file));
            return false;
        }

        DatabaseOptions options;
        DatabaseStatus status;
        if (MakeDatabase(path, options, status, error_string)) {
            chain.initError(error_string);
            return false;
        }
    }

    return true;
}

std::shared_ptr<CWallet> LoadWallet(WalletContext& context, const std::string& name, const DatabaseOptions& options, DatabaseStatus& status, bilingual_str& error, std::vector<bilingual_str>& warnings)
{
    std::shared_ptr<CWallet> wallet{new CWallet{context.chain, name, nullptr}, Deleter<CWallet>{}};
    std::shared_ptr<CWallet>* map_wallet = nullptr;
    {
        WAIT_LOCK(context.wallets_mutex, lock);
        WaitReady(context, lock, name);
        auto inserted = context.wallets.insert({name, nullptr});
        if (!inserted.second) {
            status = DatabaseStatus::FAILED_ALREADY_LOADED;
            return nullptr;
        }
        std::get_deleter<Deleter<CWallet>>(wallet)->cleanup = [&context, inserted] {
            LOCK(context.wallets_mutex);
            context.wallets.erase(inserted.first);
            context.wallets_cv.notify_all();
        };
        map_wallet = &inserted.first->second;
    }

    if (!CWallet::Open(wallet, options, status, error, warnings)) return nullptr;
    {
        LOCK(context.wallets_mutex);
        *map_wallet = wallet;
        context.wallets_cv.notify_all();
        for (auto& load_wallet : context.load_wallet_fns) {
            load_wallet(interfaces::MakeWallet(wallet));
        }
    }

    return wallet;
}

bool LoadWallets(WalletContext& context, const std::vector<std::string>& wallet_files)
{
    for (const std::string& walletFile : wallet_files) {
        DatabaseOptions options;
        options.verify = false;
        DatabaseStatus status;
        bilingual_str error;
        std::vector<bilingual_str> warnings;
        std::shared_ptr<CWallet> wallet = LoadWallet(context, walletFile, options, status, error, warnings);
        if (!wallet) {
            context.chain->initError(error);
            return false;
        }
    }
    return true;
}

void StartWallets(WalletContext& context, CScheduler& scheduler, const ArgsManager& args)
{
    for (const std::shared_ptr<CWallet>& pwallet : GetWallets(context)) {
        pwallet->postInitProcess();
    }

    // Schedule periodic wallet flushes and tx rebroadcasts
    if (args.GetBoolArg("-flushwallet", DEFAULT_FLUSHWALLET)) {
        scheduler.scheduleEvery([&context]{ MaybeCompactWalletDB(context); }, std::chrono::milliseconds{500});
    }
    scheduler.scheduleEvery([&context]{ MaybeResendWalletTxs(context); }, std::chrono::milliseconds{1000});
}

void FlushWallets(WalletContext& context)
{
    for (const std::shared_ptr<CWallet>& pwallet : GetWallets(context)) {
        pwallet->Flush();
    }
}

void StopWallets(WalletContext& context)
{
    for (const std::shared_ptr<CWallet>& pwallet : GetWallets(context)) {
        pwallet->Close();
    }
}

bool UnloadWallet(WalletContext& context, const std::string& name, bool wait, bilingual_str& error)
{
    WAIT_LOCK(context.wallets_mutex, lock);
    bool unloaded = false;
    if (std::shared_ptr<CWallet>* wallet = util::FindKey(context.wallets, name)) {
        REVERSE_LOCK(lock);
        unloaded = ReleaseWallet(std::move(*wallet));
    }
    if (wait) WaitReady(context, lock, name);
    if (!unloaded) {
        error = Untranslated(strprintf("Failed to unload wallet. Wallet '%s' is not currently loaded.", name));
        return false;
    }
    return true;
}

void UnloadWallets(WalletContext& context)
{
    WAIT_LOCK(context.wallets_mutex, lock);
    while (!context.wallets.empty()) {
        std::vector<std::shared_ptr<CWallet>> release_wallets;
        for (auto& wallet : context.wallets) {
            if (wallet.second) release_wallets.emplace_back(std::move(wallet.second));
        }
        if (!release_wallets.empty()) {
            REVERSE_LOCK(lock);
            for (auto& wallet : release_wallets) {
                ReleaseWallet(std::move(wallet));
            }
        } else {
            context.wallets_cv.wait(lock);
        }
    }
}

std::shared_ptr<CWallet> GetWallet(WalletContext& context, const std::string& name)
{
    LOCK(context.wallets_mutex);
    auto it = context.wallets.find(name);
    return it == context.wallets.end() ? nullptr : it->second;
}

std::vector<std::shared_ptr<CWallet>> GetWallets(WalletContext& context)
{
    LOCK(context.wallets_mutex);
    std::vector<std::shared_ptr<CWallet>> wallets;
    for (auto& wallet : context.wallets) {
        if (wallet.second) wallets.push_back(wallet.second);
    }
    return wallets;
}

std::unique_ptr<interfaces::Handler> HandleLoadWallet(WalletContext& context, LoadWalletFn load_wallet)
{
    LOCK(context.wallets_mutex);
    auto it = context.load_wallet_fns.emplace(context.load_wallet_fns.end(), std::move(load_wallet));
    return interfaces::MakeHandler([&context, it] { LOCK(context.wallets_mutex); context.load_wallet_fns.erase(it); });
}
