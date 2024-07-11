// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoind.h>
#include <interfaces/init.h>
#include <node/context.h>

#include <functional>
#include <string>

using node::NodeContext;

const std::function<std::string(const char*)> G_TRANSLATION_FUN = nullptr;

MAIN_FUNCTION
{
#ifdef WIN32
    common::WinCmdLineArgs winArgs;
    std::tie(argc, argv) = winArgs.get();
#endif

    NodeContext node;
    int exit_status;
    std::unique_ptr<interfaces::Init> init = interfaces::MakeNodeInit(node, argc, argv, exit_status);
    if (!init) {
        return exit_status;
    }

    return NodeMain(node, argc, argv);
}
