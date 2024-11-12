#ifndef TEST_ARGSMAN_TESTS_SETTINGS_H
#define TEST_ARGSMAN_TESTS_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using RegtestSetting = common::Setting<
    "-regtest", std::optional<char>, {.legacy = true},
    "regtest">;

using TestnetSetting = common::Setting<
    "-testnet", std::optional<char>, {.legacy = true},
    "testnet">;

#endif // TEST_ARGSMAN_TESTS_SETTINGS_H
