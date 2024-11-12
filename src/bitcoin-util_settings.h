#ifndef BITCOIN_UTIL_SETTINGS_H
#define BITCOIN_UTIL_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using VersionSetting = common::Setting<
    "-version", common::Unset, common::SettingOptions{.legacy = true},
    "Print version and exit">;

#endif // BITCOIN_UTIL_SETTINGS_H
