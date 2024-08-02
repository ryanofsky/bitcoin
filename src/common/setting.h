// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_SETTING_H
#define BITCOIN_COMMON_SETTING_H

#include <common/args.h>

#include <tuple>

namespace common {
//! State representing a setting that is unset
struct Unset {};
//! State representing a setting that is enabled without a value ("-setting")
struct Enabled {};
//! State representing a setting that is disabled ("-nosetting")
struct Disabled {};

struct SettingOptions {
    bool disallow_multiple{false};
    bool disallow_negation{false};
    bool disallow_empty_string{false};
    bool empty_string_means_unset{true};
    bool disabled_list_means_clear{true};
};

template<typename... Args>
struct HelpArgs
{
    HelpArgs(Args... help_args) : args{help_args...} {}
    std::tuple<Args...> args;
};

template<typename... Args>
struct CheckArgs : std::tuple<Args...>
{
    CheckArgs(Args... check_args) : args{check_args...} {}
    std::tuple<Args...> args;
};

template<size_t N>
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }
    char value[N];
};

namespace internal {
template<SettingOptions options, typename T>
void SettingRegister(auto& manager, auto summary, auto help, auto category, auto check_fn, auto default_fn, auto&&... register_options);
template<SettingOptions options, typename T>
T SettingConstruct(auto default_fn);
template<SettingOptions options, typename T>
void SettingUpdate(auto& manager, auto summary, auto default_fn, T& out);
};

template<StringLiteral summary, StringLiteral help, typename T, auto category, auto check_fn = nullptr, auto default_fn = nullptr, SettingOptions options = SettingOptions{}>
struct Setting {
    static void Register(auto& manager, auto&&... register_options)
    {
        internal::SettingRegister<options, T>(manager, summary.value, help.value, category, check_fn, default_fn, register_options...);
    }

    static T Construct()
    {
        return internal::SettingConstruct<options, T>(default_fn);
    }

    static void Update(auto& manager, T& out)
    {
        internal::SettingUpdate<options, T>(manager, summary.value, default_fn, out);
    }

    static T Get(auto& manager)
    {
        auto value{Construct()};
        Update(manager, value);
        return value;
    }

    // Convenient accessors to set template paramaters by name
    template<auto _check_fn>
    using CheckFn = Setting<summary, help, T, category, _check_fn, default_fn, options>;
    template<auto _default_fn>
    using DefaultFn = Setting<summary, help, T, category, check_fn, _default_fn, options>;
    template<auto _default>
    using Default = Setting<summary, help, T, category, check_fn, +[] { return _default; }, options>;
    template<auto _options>
    using Options = Setting<summary, help, T, category, check_fn, default_fn, _options>;
};
} // namespace common

#include <common/setting_internal.h>

#endif // BITCOIN_COMMON_SETTING_H
