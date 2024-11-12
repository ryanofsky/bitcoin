#ifndef TEST_ARGSMAN_TESTS_SETTINGS_H
#define TEST_ARGSMAN_TESTS_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using RegtestSetting = common::Setting<
    "-regtest", common::Unset, {.legacy = true},
    "regtest">;

using TestnetSetting = common::Setting<
    "-testnet", common::Unset, {.legacy = true},
    "testnet">;

#endif // TEST_ARGSMAN_TESTS_SETTINGS_H
