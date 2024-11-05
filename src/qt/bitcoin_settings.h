#ifndef QT_BITCOIN_SETTINGS_H
#define QT_BITCOIN_SETTINGS_H

#include <common/setting.h>

#include <string>
#include <vector>

using ChoosedatadirSetting = common::Setting<
    "-choosedatadir", bool, {.legacy = true},
    "Choose data directory on startup (default: %u)">
    ::Default<DEFAULT_CHOOSE_DATADIR>
    ::Category<OptionsCategory::GUI>;

using LangSetting = common::Setting<
    "-lang=<lang>", std::string, {.legacy = true},
    "Set language, for example \"de_DE\" (default: system locale)">
    ::Default<lang_territory.toStdString()>
    ::HelpArgs<>
    ::Category<OptionsCategory::GUI>;

using MinSetting = common::Setting<
    "-min", bool, {.legacy = true},
    "Start minimized">
    ::Category<OptionsCategory::GUI>;

using ResetguisettingsSetting = common::Setting<
    "-resetguisettings", bool, {.legacy = true},
    "Reset all settings changed in the GUI">
    ::Category<OptionsCategory::GUI>;

using SplashSetting = common::Setting<
    "-splash", bool, {.legacy = true},
    "Show splash screen on startup (default: %u)">
    ::Default<DEFAULT_SPLASHSCREEN>
    ::Category<OptionsCategory::GUI>;

using UiplatformSetting = common::Setting<
    "-uiplatform", std::string, {.legacy = true, .debug_only = true},
    "Select platform to customize UI for (one of windows, macosx, other; default: %s)">
    ::Default<BitcoinGUI::DEFAULT_UIPLATFORM>
    ::Category<OptionsCategory::GUI>;

#endif // QT_BITCOIN_SETTINGS_H
