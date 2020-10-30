// Copyright (c) 2017-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet/walletutil.h>

#include <logging.h>
#include <util/system.h>

fs::path GetWalletDir()
{
    fs::path path;

    if (gArgs.IsArgSet("-walletdir")) {
        path = gArgs.GetArg("-walletdir", "");
        if (!fs::is_directory(path)) {
            // If the path specified doesn't exist, we return the deliberately
            // invalid empty string.
            path = "";
        }
    } else {
        path = GetDataDir();
        // If a wallets directory exists, use that, otherwise default to GetDataDir
        if (fs::is_directory(path / "wallets")) {
            path /= "wallets";
        }
    }

    return path;
}

bool IsFeatureSupported(int wallet_version, int feature_version)
{
    return wallet_version >= feature_version;
}

WalletFeature GetClosestWalletFeature(int version)
{
    if (version >= FEATURE_LATEST) return FEATURE_LATEST;
    if (version >= FEATURE_PRE_SPLIT_KEYPOOL) return FEATURE_PRE_SPLIT_KEYPOOL;
    if (version >= FEATURE_NO_DEFAULT_KEY) return FEATURE_NO_DEFAULT_KEY;
    if (version >= FEATURE_HD_SPLIT) return FEATURE_HD_SPLIT;
    if (version >= FEATURE_HD) return FEATURE_HD;
    if (version >= FEATURE_COMPRPUBKEY) return FEATURE_COMPRPUBKEY;
    if (version >= FEATURE_WALLETCRYPT) return FEATURE_WALLETCRYPT;
    if (version >= FEATURE_BASE) return FEATURE_BASE;
    return static_cast<WalletFeature>(0);
}
