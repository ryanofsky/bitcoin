#ifndef BITCOIN_WALLET_SETTINGS_H
#define BITCOIN_WALLET_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using VersionSetting = common::Setting<
    "-version", std::optional<char>, {.legacy = true},
    "Print version and exit">;

using DatadirSetting = common::Setting<
    "-datadir=<dir>", std::string, {.legacy = true},
    "Specify data directory">;

using WalletSetting = common::Setting<
    "-wallet=<wallet-name>", std::optional<std::string>, {.legacy = true, .network_only = true},
    "Specify wallet name">;

using DumpfileSetting = common::Setting<
    "-dumpfile=<file name>", std::optional<char>, {.legacy = true, .disallow_negation = true},
    "When used with 'dump', writes out the records to this file. When used with 'createfromdump', loads the records into a new wallet.">;

using DebugSetting = common::Setting<
    "-debug=<category>", bool, {.legacy = true},
    "Output debugging information (default: 0).">
    ::Category<OptionsCategory::DEBUG_TEST>;

using DescriptorsSetting = common::Setting<
    "-descriptors", std::optional<bool>, {.legacy = true},
    "Create descriptors wallet. Only for 'create'">;

using LegacySetting = common::Setting<
    "-legacy", std::optional<bool>, {.legacy = true},
    "Create legacy wallet. Only for 'create'">;

using FormatSetting = common::Setting<
    "-format=<format>", std::optional<char>, {.legacy = true},
    "The format of the wallet file to create. Either \"bdb\" or \"sqlite\". Only used with 'createfromdump'">;

using PrinttoconsoleSetting = common::Setting<
    "-printtoconsole", bool, {.legacy = true},
    "Send trace/debug info to console (default: 1 when no -debug is true, 0 otherwise).">
    ::Default<args.GetBoolArg("-debug", false)>
    ::HelpArgs<>
    ::Category<OptionsCategory::DEBUG_TEST>;

using WithinternalbdbSetting = common::Setting<
    "-withinternalbdb", bool, {.legacy = true},
    "Use the internal Berkeley DB parser when dumping a Berkeley DB wallet file (default: false)">
    ::Category<OptionsCategory::DEBUG_TEST>;

#endif // BITCOIN_WALLET_SETTINGS_H
