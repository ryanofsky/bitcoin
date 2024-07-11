// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <config/bitcoin-config.h> // IWYU pragma: keep

#include <chainparams.h>
#include <chainparamsbase.h>
#include <clientversion.h>
#include <common/args.h>
#include <common/system.h>
#include <compat/compat.h>
#include <init/common.h>
#include <interfaces/init.h>
#include <interfaces/ipc.h>
#include <tinyformat.h>
#include <util/translation.h>

static const char* const HELP_USAGE{R"(
bitcoin-mine is a test program for interacting with bitcoin-node via IPC.

Usage:
  bitcoin-mine [options] [--] [node options]
)"};

static const char* HELP_EXAMPLES{R"(
Examples:
  # Start separate bitcoin-node that bitcoin-mine can connect to.
  bitcoin-node -regtest -ipcbind=unix

  # Connect to existing bitcoin-node or spawn new one if not running.
  bitcoin-mine -regtest

  # Stop bitcoin node.
  bitcoin-mine -regtest -stop;

  # Run with debug output.
  bitcoin-mine -regtest -debug

  # Pass extra options to bitcoin-node when spawning it
  bitcoin-mine -regtest -- -upnp
)"};

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

static void AddArgs(ArgsManager& args)
{
    SetupHelpOptions(args);
    SetupChainParamsBaseOptions(args);
    args.AddArg("-version", "Print version and exit", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    args.AddArg("-datadir=<dir>", "Specify data directory", ArgsManager::ALLOW_ANY, OptionsCategory::OPTIONS);
    args.AddArg("-debug=<category>", "Output debugging information (default: 0).", ArgsManager::ALLOW_ANY, OptionsCategory::DEBUG_TEST);
    args.AddArg("-stop", "Stop bitcoin-node process if it is running.", ArgsManager::ALLOW_ANY | ArgsManager::NETWORK_ONLY, OptionsCategory::OPTIONS);
    args.AddArg("-ipcconnect=<address>", "Connect to bitcoin-node process in the background to perform online operations. Valid <address> values are 'auto' to try connecting to default socket in <datadir>/sockets/node.sock, but and spawn a node if it isn't available, 'unix' to connect to the default socket and fail if it isn't available, 'unix:<socket path>' to connect to a socket at a nonstandard path, and -noipcconnect to not try to connect. Default value: auto", ArgsManager::ALLOW_ANY, OptionsCategory::IPC);
}

MAIN_FUNCTION
{
    // Look for -- separator for arguments which should be passed to bitcoin-node.
    int argc_mine{argc};
    for (int i{0}; i < argc; ++i) {
       if (std::string_view{argv[i]} == "--") argc_mine = i;
    }

    ArgsManager& args = gArgs;
    AddArgs(args);
    std::string error_message;
    if (!args.ParseParameters(argc_mine, argv, error_message)) {
        tfm::format(std::cerr, "Error parsing command line arguments: %s\n", error_message);
        return EXIT_FAILURE;
    }
    if (!args.ReadConfigFiles(error_message, true)) {
        tfm::format(std::cerr, "Error reading config files: %s\n", error_message);
        return EXIT_FAILURE;
    }
    if (HelpRequested(args) || args.IsArgSet("-version")) {
        std::string output{strprintf("%s bitcoin-mine version", PACKAGE_NAME) + " " + FormatFullVersion() + "\n"};
        if (args.IsArgSet("-version")) {
            output += FormatParagraph(LicenseInfo());
        } else {
            output += HELP_USAGE;
            output += args.GetHelpMessage();
            output += HELP_EXAMPLES;
        }
        tfm::format(std::cout, "%s", output);
        return EXIT_SUCCESS;
    }

    // check for printtoconsole, allow -debug
    LogInstance().m_print_to_console = args.GetBoolArg("-printtoconsole", args.GetBoolArg("-debug", false));

    if (!CheckDataDirOption(args)) {
        tfm::format(std::cerr, "Error: Specified data directory \"%s\" does not exist.\n", args.GetArg("-datadir", ""));
        return EXIT_FAILURE;
    }
    SelectParams(args.GetChainType());
    if (!init::StartLogging(gArgs)) {
        tfm::format(std::cerr, "Error: StartLogging failed\n");
        return EXIT_FAILURE;
    }

    // Connect to bitcoin-node process or spawn.
    std::unique_ptr<interfaces::Init> mine_init{interfaces::MakeMineInit(argc, argv)};
    assert(mine_init);
    std::string address{args.GetArg("-ipcconnect", "auto")};
    std::unique_ptr<interfaces::Init> node_init{mine_init->ipc()->connectAddress(address)};
    bool spawn{!node_init};
    if (spawn) {
        tfm::format(std::cout, "Spawning bitcoin-node\n");
        node_init = mine_init->ipc()->spawnProcess("bitcoin-node", /*detach=*/true);
        assert(node_init);
    } else {
        tfm::format(std::cout, "Connected to bitcoin-node\n");
    }
    std::unique_ptr<interfaces::Mining> mining{node_init->makeMining()};
    assert(mining);
    if (spawn) {
        args.LockSettings([&](common::Settings& settings) {
            const int node_argc{argc - std::min(argc, argc_mine + 1)};
            const char* const* node_argv{argv + argc - node_argc};
            mining->startNode(settings, node_argc, node_argv);
        });
    }

    std::optional<uint256> tip_hash{mining->getTipHash()};
    if (tip_hash) {
        tfm::format(std::cout, "Tip hash is %s.\n", tip_hash->ToString());
    } else {
        tfm::format(std::cout, "Tip hash is null.\n");
    }

    if (args.GetBoolArg("-stop", false)) {
        tfm::format(std::cout, "Stopping bitcoin-node.\n");
        int exit_status;
        mining->stopNode(exit_status);
        tfm::format(std::cout, "bitcoin-node exited with status %i.\n", exit_status);
    }

    return EXIT_SUCCESS;
}
