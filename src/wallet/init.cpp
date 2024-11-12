// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin-build-config.h> // IWYU pragma: keep

#include <common/args.h>
#include <init.h>
#include <init_settings.h>
#include <interfaces/chain.h>
#include <interfaces/init.h>
#include <interfaces/wallet.h>
#include <net.h>
#include <node/context.h>
#include <node/interface_ui.h>
#include <outputtype.h>
#include <univalue.h>
#include <util/check.h>
#include <util/moneystr.h>
#include <util/translation.h>
<<<<<<< HEAD
||||||| parent of b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
#ifdef USE_BDB
#include <wallet/bdb.h>
#endif
=======
#include <wallet/init_settings.h>
#ifdef USE_BDB
#include <wallet/bdb.h>
#endif
>>>>>>> b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
#include <wallet/coincontrol.h>
#include <wallet/wallet.h>
#include <walletinitinterface.h>

using node::NodeContext;

namespace wallet {
class WalletInit : public WalletInitInterface
{
public:
    //! Was the wallet component compiled in.
    bool HasWalletSupport() const override {return true;}

    //! Return the wallets help message.
    void AddWalletOptions(ArgsManager& argsman) const override;

    //! Wallets parameter interaction
    bool ParameterInteraction() const override;

    //! Add wallets that should be opened to list of chain clients.
    void Construct(NodeContext& node) const override;
};

void WalletInit::AddWalletOptions(ArgsManager& argsman) const
{
<<<<<<< HEAD
    argsman.AddArg("-addresstype", strprintf("What type of addresses to use (%s, default: \"%s\")", FormatAllOutputTypes(), FormatOutputType(DEFAULT_ADDRESS_TYPE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-avoidpartialspends", strprintf("Group outputs by address, selecting many (possibly all) or none, instead of selecting on a per-output basis. Privacy is improved as addresses are mostly swept with fewer transactions and outputs are aggregated in clean change addresses. It may result in higher fees due to less optimal coin selection caused by this added limitation and possibly a larger-than-necessary number of inputs being used. Always enabled for wallets with \"avoid_reuse\" enabled, otherwise default: %u.", DEFAULT_AVOIDPARTIALSPENDS), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-changetype",
                   strprintf("What type of change to use (%s). Default is \"legacy\" when "
                   "-addresstype=legacy, else it is an implementation detail.", FormatAllOutputTypes()),
                   ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-consolidatefeerate=<amt>", strprintf("The maximum feerate (in %s/kvB) at which transaction building may use more inputs than strictly necessary so that the wallet's UTXO pool can be reduced (default: %s).", CURRENCY_UNIT, FormatMoney(DEFAULT_CONSOLIDATE_FEERATE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-disablewallet", "Do not load the wallet and disable wallet RPC calls", ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-discardfee=<amt>", strprintf("The fee rate (in %s/kvB) that indicates your tolerance for discarding change by adding it to the fee (default: %s). "
                                                                "Note: An output is discarded if it is dust at this rate, but we will always discard up to the dust relay fee and a discard fee above that is limited by the fee estimate for the longest target",
                                                              CURRENCY_UNIT, FormatMoney(DEFAULT_DISCARD_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
||||||| parent of b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
    argsman.AddArg("-addresstype", strprintf("What type of addresses to use (\"legacy\", \"p2sh-segwit\", \"bech32\", or \"bech32m\", default: \"%s\")", FormatOutputType(DEFAULT_ADDRESS_TYPE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-avoidpartialspends", strprintf("Group outputs by address, selecting many (possibly all) or none, instead of selecting on a per-output basis. Privacy is improved as addresses are mostly swept with fewer transactions and outputs are aggregated in clean change addresses. It may result in higher fees due to less optimal coin selection caused by this added limitation and possibly a larger-than-necessary number of inputs being used. Always enabled for wallets with \"avoid_reuse\" enabled, otherwise default: %u.", DEFAULT_AVOIDPARTIALSPENDS), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-changetype",
                   "What type of change to use (\"legacy\", \"p2sh-segwit\", \"bech32\", or \"bech32m\"). Default is \"legacy\" when "
                   "-addresstype=legacy, else it is an implementation detail.",
                   ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-consolidatefeerate=<amt>", strprintf("The maximum feerate (in %s/kvB) at which transaction building may use more inputs than strictly necessary so that the wallet's UTXO pool can be reduced (default: %s).", CURRENCY_UNIT, FormatMoney(DEFAULT_CONSOLIDATE_FEERATE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-disablewallet", "Do not load the wallet and disable wallet RPC calls", ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-discardfee=<amt>", strprintf("The fee rate (in %s/kvB) that indicates your tolerance for discarding change by adding it to the fee (default: %s). "
                                                                "Note: An output is discarded if it is dust at this rate, but we will always discard up to the dust relay fee and a discard fee above that is limited by the fee estimate for the longest target",
                                                              CURRENCY_UNIT, FormatMoney(DEFAULT_DISCARD_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
=======
    AddressTypeSetting::Register(argsman);
    AvoidPartialSpendsSetting::Register(argsman);
    ChangeTypeSetting::Register(argsman);
    ConsolidateFeeRateSetting::Register(argsman);
    DisableWalletSetting::Register(argsman);
    DiscardFeeSetting::Register(argsman);
>>>>>>> b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)

<<<<<<< HEAD
    argsman.AddArg("-fallbackfee=<amt>", strprintf("A fee rate (in %s/kvB) that will be used when fee estimation has insufficient data. 0 to entirely disable the fallbackfee feature. (default: %s)",
                                                               CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-keypool=<n>", strprintf("Set key pool size to <n> (default: %u). Warning: Smaller sizes may increase the risk of losing funds when restoring from an old backup, if none of the addresses in the original keypool have been used.", DEFAULT_KEYPOOL_SIZE), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-maxapsfee=<n>", strprintf("Spend up to this amount in additional (absolute) fees (in %s) if it allows the use of partial spend avoidance (default: %s)", CURRENCY_UNIT, FormatMoney(DEFAULT_MAX_AVOIDPARTIALSPEND_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-maxtxfee=<amt>", strprintf("Maximum total fees (in %s) to use in a single wallet transaction; setting this too low may abort large transactions (default: %s)",
        CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MAXFEE)), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-mintxfee=<amt>", strprintf("Fee rates (in %s/kvB) smaller than this are considered zero fee for transaction creation (default: %s)",
                                                            CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MINFEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-paytxfee=<amt>", strprintf("(DEPRECATED) Fee rate (in %s/kvB) to add to transactions you send (default: %s)",
                                                            CURRENCY_UNIT, FormatMoney(CFeeRate{DEFAULT_PAY_TX_FEE}.GetFeePerK())), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
||||||| parent of b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
    argsman.AddArg("-fallbackfee=<amt>", strprintf("A fee rate (in %s/kvB) that will be used when fee estimation has insufficient data. 0 to entirely disable the fallbackfee feature. (default: %s)",
                                                               CURRENCY_UNIT, FormatMoney(DEFAULT_FALLBACK_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-keypool=<n>", strprintf("Set key pool size to <n> (default: %u). Warning: Smaller sizes may increase the risk of losing funds when restoring from an old backup, if none of the addresses in the original keypool have been used.", DEFAULT_KEYPOOL_SIZE), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-maxapsfee=<n>", strprintf("Spend up to this amount in additional (absolute) fees (in %s) if it allows the use of partial spend avoidance (default: %s)", CURRENCY_UNIT, FormatMoney(DEFAULT_MAX_AVOIDPARTIALSPEND_FEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-maxtxfee=<amt>", strprintf("Maximum total fees (in %s) to use in a single wallet transaction; setting this too low may abort large transactions (default: %s)",
        CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MAXFEE)), ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    argsman.AddArg("-mintxfee=<amt>", strprintf("Fee rates (in %s/kvB) smaller than this are considered zero fee for transaction creation (default: %s)",
                                                            CURRENCY_UNIT, FormatMoney(DEFAULT_TRANSACTION_MINFEE)), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
    argsman.AddArg("-paytxfee=<amt>", strprintf("Fee rate (in %s/kvB) to add to transactions you send (default: %s)",
                                                            CURRENCY_UNIT, FormatMoney(CFeeRate{DEFAULT_PAY_TX_FEE}.GetFeePerK())), ArgsManager::ALLOW_ANY, OptionsCategory::WALLET);
=======
    FallbackFeeSetting::Register(argsman);
    KeyPoolSetting::Register(argsman);
    MaxApsFeeSetting::Register(argsman);
    MaxTxFeeSetting::Register(argsman);
    MinTxFeeSetting::Register(argsman);
    PayTxFeeSetting::Register(argsman);
>>>>>>> b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
#ifdef ENABLE_EXTERNAL_SIGNER
    SignerSetting::Register(argsman);
#endif
    SpendZeroConfChangeSetting::Register(argsman);
    TxConfirmTargetSetting::Register(argsman);
    WalletSetting::Register(argsman);
    WalletBroadcastSetting::Register(argsman);
    WalletDirSetting::Register(argsman);
#if HAVE_SYSTEM
    WalletNotifySetting::Register(argsman);
#endif
    WalletRbfSetting::Register(argsman);

<<<<<<< HEAD
    argsman.AddArg("-unsafesqlitesync", "Set SQLite synchronous=OFF to disable waiting for the database to sync to disk. This is unsafe and can cause data loss and corruption. This option is only used by tests to improve their performance (default: false)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);
||||||| parent of b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
#ifdef USE_BDB
    argsman.AddArg("-dblogsize=<n>", strprintf("Flush wallet database activity from memory to disk log every <n> megabytes (default: %u)", DatabaseOptions().max_log_mb), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);
    argsman.AddArg("-flushwallet", strprintf("Run a thread to flush wallet periodically (default: %u)", DEFAULT_FLUSHWALLET), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);
    argsman.AddArg("-privdb", strprintf("Sets the DB_PRIVATE flag in the wallet db environment (default: %u)", !DatabaseOptions().use_shared_memory), ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);
    argsman.AddArg("-swapbdbendian", "Swaps the internal endianness of BDB wallet databases (default: false)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);
#else
    argsman.AddHiddenArgs({"-dblogsize", "-flushwallet", "-privdb", "-swapbdbendian"});
#endif

#ifdef USE_SQLITE
    argsman.AddArg("-unsafesqlitesync", "Set SQLite synchronous=OFF to disable waiting for the database to sync to disk. This is unsafe and can cause data loss and corruption. This option is only used by tests to improve their performance (default: false)", ArgsManager::ALLOW_ANY | ArgsManager::DEBUG_ONLY, OptionsCategory::WALLET_DEBUG_TEST);
#else
    argsman.AddHiddenArgs({"-unsafesqlitesync"});
#endif
=======
#ifdef USE_BDB
    DbLogSizeSetting::Register(argsman);
    FlushWalletSetting::Register(argsman);
    PrivDbSetting::Register(argsman);
    SwapBdbEndianSetting::Register(argsman);
#else
    DbLogSizeSetting::Hidden::Register(argsman);
    FlushWalletSetting::Hidden::Register(argsman);
    PrivDbSetting::Hidden::Register(argsman);
    SwapBdbEndianSetting::Hidden::Register(argsman);
#endif

#ifdef USE_SQLITE
    UnsafeSqliteSyncSetting::Register(argsman);
#else
    UnsafeSqliteSyncSetting::Hidden::Register(argsman);
#endif
>>>>>>> b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)

    WalletRejectLongChainsSetting::Register(argsman);
    WalletCrossChainSetting::Register(argsman);
}

bool WalletInit::ParameterInteraction() const
{
<<<<<<< HEAD
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        for (const std::string& wallet : gArgs.GetArgs("-wallet")) {
            LogInfo("Parameter interaction: -disablewallet -> ignoring -wallet=%s", wallet);
||||||| parent of b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
#ifdef USE_BDB
     if (!BerkeleyDatabaseSanityCheck()) {
         return InitError(Untranslated("A version conflict was detected between the run-time BerkeleyDB library and the one used during compilation."));
     }
#endif
    if (gArgs.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        for (const std::string& wallet : gArgs.GetArgs("-wallet")) {
            LogPrintf("%s: parameter interaction: -disablewallet -> ignoring -wallet=%s\n", __func__, wallet);
=======
#ifdef USE_BDB
     if (!BerkeleyDatabaseSanityCheck()) {
         return InitError(Untranslated("A version conflict was detected between the run-time BerkeleyDB library and the one used during compilation."));
     }
#endif
    if (DisableWalletSetting::Get(gArgs)) {
        for (const std::string& wallet : WalletSetting::Get(gArgs)) {
            LogPrintf("%s: parameter interaction: -disablewallet -> ignoring -wallet=%s\n", __func__, wallet);
>>>>>>> b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
        }

        return true;
    }

<<<<<<< HEAD
    if (gArgs.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY) && gArgs.SoftSetBoolArg("-walletbroadcast", false)) {
        LogInfo("Parameter interaction: -blocksonly=1 -> setting -walletbroadcast=0");
||||||| parent of b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
    if (gArgs.GetBoolArg("-blocksonly", DEFAULT_BLOCKSONLY) && gArgs.SoftSetBoolArg("-walletbroadcast", false)) {
        LogPrintf("%s: parameter interaction: -blocksonly=1 -> setting -walletbroadcast=0\n", __func__);
=======
    if (BlocksOnlySetting::Get(gArgs, DEFAULT_BLOCKSONLY) && gArgs.SoftSetBoolArg("-walletbroadcast", false)) {
        LogPrintf("%s: parameter interaction: -blocksonly=1 -> setting -walletbroadcast=0\n", __func__);
>>>>>>> b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
    }

    return true;
}

void WalletInit::Construct(NodeContext& node) const
{
    ArgsManager& args = *Assert(node.args);
<<<<<<< HEAD
    if (args.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        LogInfo("Wallet disabled!");
||||||| parent of b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
    if (args.GetBoolArg("-disablewallet", DEFAULT_DISABLE_WALLET)) {
        LogPrintf("Wallet disabled!\n");
=======
    if (DisableWalletSetting::Get(args)) {
        LogPrintf("Wallet disabled!\n");
>>>>>>> b3968352b292 (scripted-diff: Replace AddArgs / GetArgs calls with Setting Register / Get calls)
        return;
    }
    auto wallet_loader = node.init->makeWalletLoader(*node.chain);
    node.wallet_loader = wallet_loader.get();
    node.chain_clients.emplace_back(std::move(wallet_loader));
}
} // namespace wallet

const WalletInitInterface& g_wallet_init_interface = wallet::WalletInit();
