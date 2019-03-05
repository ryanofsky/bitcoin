// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/settings.h>

#include <univalue.h>

namespace util {
namespace {

struct Source {
    SettingsSpan span;
    bool forced = false;
    bool config_file = false;
    bool config_file_default_section = false;

    explicit Source(SettingsSpan span) : span(span) {}
    Source& SetForced() noexcept { forced = true; return *this; }
    Source& SetConfigFile(bool default_section) noexcept { config_file = true; config_file_default_section = default_section; return *this; }
};

template <typename Fn>
static void MergeSettings(const Settings& settings, const std::string& section, const std::string& name, Fn&& fn)
{
    if (auto* value = FindKey(settings.forced_settings, name)) {
        fn(Source(SettingsSpan(*value)).SetForced());
    }
    if (auto* values = FindKey(settings.command_line_options, name)) {
        fn(Source(SettingsSpan(*values)));
    }
    if (!section.empty()) {
        if (auto* map = FindKey(settings.ro_config, section)) {
            if (auto* values = FindKey(*map, name)) {
                fn(Source(SettingsSpan(*values)).SetConfigFile(/* default_section= */ false));
            }
        }
    }
    if (auto* map = FindKey(settings.ro_config, "")) {
        if (auto* values = FindKey(*map, name)) {
            fn(Source(SettingsSpan(*values)).SetConfigFile(/* default_section= */ true));
        }
    }
}
} // namespace

SettingsValue GetSetting(const Settings& settings,
    const std::string& section,
    const std::string& name,
    bool ignore_default_section_config,
    bool get_chain_name)
{
    SettingsValue result;
    MergeSettings(settings, section, name, [&](Source source) {
        // Weird behavior preserved for backwards compatibility: Apply negated
        // setting in otherwise ignored sections. A negated value in the
        // default section is applied to network specific options, even though
        // non-negated values there would be ignored.
        const bool never_ignore_negated_setting = source.span.last_negated();

        // Weird behavior preserved for backwards compatibility: Take first
        // assigned value instead of last. In general, later settings take
        // precedence over early settings, but for backwards compatibility in
        // the config file the precedence is reversed for most settings.
        const bool reverse_precedence = source.config_file && !get_chain_name;

        // Weird behavior preserved for backwards compatibility: Negated
        // -regtest and -testnet arguments which you would expect to override
        // values set in the configuration file are currently accepted but
        // silently ignored. It would be better to apply these just like other
        // negated values, or at least warn they are ignored.
        const bool skip_negated_command_line = get_chain_name;

        // Ignore settings in default config section if requested.
        if (ignore_default_section_config && source.config_file_default_section && !never_ignore_negated_setting) return;

        // Skip negated command line settings.
        if (skip_negated_command_line && source.span.last_negated()) return;

        // Stick with highest priority value, keeping result if already set.
        if (!result.isNull()) return;

        if (!source.span.empty()) {
            result = reverse_precedence ? source.span.begin()[0] : source.span.end()[-1];
        } else if (source.span.last_negated()) {
            result = false;
        }
    });
    return result;
}

std::vector<SettingsValue> GetSettingsList(const Settings& settings,
    const std::string& section,
    const std::string& name,
    bool ignore_default_section_config)
{
    std::vector<SettingsValue> result;
    bool result_complete = false;
    bool prev_negated_empty = false;
    MergeSettings(settings, section, name, [&](Source source) {
        // Weird behavior preserved for backwards compatibility: Apply config
        // file settings even if negated on command line. Negating a setting on
        // command line will discard earlier settings on the command line and
        // settings in the config file, unless the negated command line value is
        // followed by non-negated value, in which case config file settings
        // will be brought back from the dead (but earlier command line settings
        // will still be discarded).
        const bool add_zombie_config_values = source.config_file && !prev_negated_empty;

        // Ignore settings in default config section if requested.
        if (ignore_default_section_config && source.config_file_default_section) return;

        // Add new settings to the result if isn't already complete, or if the
        // values are zombies.
        if (!result_complete || add_zombie_config_values) {
            for (const auto& value : source.span) {
                if (value.isArray()) {
                    result.insert(result.end(), value.getValues().begin(), value.getValues().end());
                } else {
                    result.push_back(value);
                }
            }
        }

        // If a setting was negated, or if a setting was forced, set
        // result_complete to true to ignore any later lower priority settings.
        result_complete |= source.span.negated() > 0 || source.forced;

        // Update the negated and empty state used for the zombie values check.
        prev_negated_empty |= source.span.last_negated() && result.empty();
    });
    return result;
}

bool OnlyHasDefaultSectionSetting(const Settings& settings, const std::string& section, const std::string& name)
{
    bool has_default_section_setting = false;
    bool has_other_setting = false;
    MergeSettings(settings, section, name, [&](Source source) {
        if (source.span.empty()) return;
        else if (source.config_file_default_section) has_default_section_setting = true;
        else has_other_setting = true;
    });
    // If a value is set in the default section and not explicitly overwritten by the
    // user on the command line or in a different section, then we want to enable
    // warnings about the value being ignored.
    return has_default_section_setting && !has_other_setting;
}

SettingsSpan::SettingsSpan(const std::vector<SettingsValue>& vec) noexcept : SettingsSpan(vec.data(), vec.size()) {}
const SettingsValue* SettingsSpan::begin() const { return data + negated(); }
const SettingsValue* SettingsSpan::end() const { return data + size; }
bool SettingsSpan::empty() const { return size == 0 || last_negated(); }
bool SettingsSpan::last_negated() const { return size > 0 && data[size - 1].isFalse(); }
size_t SettingsSpan::negated() const
{
    for (size_t i = size; i > 0; --i) {
        if (data[i - 1].isFalse()) return i; // Return number of negated values (position of last false value)
    }
    return 0;
}

} // namespace util
