// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_SETTING_H
#define BITCOIN_COMMON_SETTING_H

#include <common/args.h>

#include <type_traits>
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
struct ParseArgs
{
    ParseArgs(Args... parse_args) : args{parse_args...} {}
    std::tuple<Args...> args;
};

template<size_t N>
struct StringLiteral {
    constexpr StringLiteral(const char (&str)[N]) {
        std::copy_n(str, N, value);
    }
    char value[N];
};

template<auto struct_field>
struct Field;

namespace internal {
template<SettingOptions options, typename T>
void SettingRegister(auto& manager, auto summary, auto help, auto category, auto parse_fn, auto default_fn, auto&&... register_options);
template<SettingOptions options, typename T>
T SettingConstruct(auto default_fn);
template<SettingOptions options, typename T, typename ParseType>
bool SettingUpdate(auto& manager, auto summary, auto default_fn, auto get_fn, T& out, std::string& error);

template<typename T>
struct MemPtrTraits;

template<typename T, typename U>
struct MemPtrTraits<T U::*>
{
    using field_t = T;
    using struct_t = U;
};

template<typename T>
struct FieldTraits
{
    using setting_t = T;
    static constexpr auto struct_field = nullptr;
};

template<auto _struct_field>
struct FieldTraits<Field<_struct_field>>
{
    using mem_ptr_t = MemPtrTraits<decltype(_struct_field)>;
    using setting_t = mem_ptr_t::field_t;
    static constexpr auto struct_field = _struct_field;
};
} // namespace internal

template<StringLiteral summary, StringLiteral help, typename T, auto category, SettingOptions options = SettingOptions{}, typename ParseType = void, auto parse_fn = nullptr, auto default_fn = nullptr, auto get_fn = nullptr>
struct Setting {
    using field_t = internal::FieldTraits<T>;
    using setting_t = field_t::setting_t;
    using parse_t = std::conditional_t<std::is_same_v<ParseType, void>, setting_t, ParseType>;

    static void Register(auto& manager, auto&&... register_options)
    {
        internal::SettingRegister<options, parse_t>(manager, summary.value, help.value, category, parse_fn, default_fn, register_options...);
    }

    static setting_t Construct()
    {
        return internal::SettingConstruct<options, setting_t>(default_fn);
    }

    static void Update(auto& manager, setting_t& out)
    {
        std::string error;
        if (!internal::SettingUpdate<options, setting_t, parse_t>(manager, summary.value, default_fn, get_fn, out, error)) {
            throw std::runtime_error{std::move(error)};
        }
    }

    static setting_t Get(auto& manager)
    {
        auto value{Construct()};
        Update(manager, value);
        return value;
    }

    // Convenient accessors to set template parameters by name
    template<auto _options>
    using Options = Setting<summary, help, T, category, _options, ParseType, parse_fn, default_fn, get_fn>;
    template<typename _ParseType>
    using ParseAs = Setting<summary, help, T, category, options, _ParseType, parse_fn, default_fn, get_fn>;
    template<auto _parse_fn>
    using ParseFn = Setting<summary, help, T, category, options, ParseType, _parse_fn, default_fn, get_fn>;
    template<auto _default>
    using Default = Setting<summary, help, T, category, options, ParseType, parse_fn, +[] { return _default; }, get_fn>;
    template<auto _default_fn>
    using DefaultFn = Setting<summary, help, T, category, options, ParseType, parse_fn, _default_fn, get_fn>;
    template<auto _get_fn>
    using GetFn = Setting<summary, help, T, category, options, ParseType, parse_fn, default_fn, _get_fn>;
};
} // namespace common

#include <common/setting_internal.h>

#endif // BITCOIN_COMMON_SETTING_H
