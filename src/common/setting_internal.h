// Copyright (c) 2024 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_COMMON_SETTING_INTERNAL_H
#define BITCOIN_COMMON_SETTING_INTERNAL_H

#include <common/args.h>
#include <common/settings.h>
#include <univalue.h>
#include <util/strencodings.h>

namespace common {
namespace internal {
template<typename T>
struct SettingTraitsBase
{
    using setting_t = T;
    using number_t = void;
    static constexpr bool has_unset{false};
    static constexpr bool has_enabled{false};
    static constexpr bool has_disabled{false};
    static constexpr bool has_bool{false};
    static constexpr bool has_number{false};
    static constexpr bool has_string{false};
    static constexpr bool has_list{false};
    static setting_t Construct() { return {}; }
};

template<typename T, SettingOptions options>
struct SettingTraits;

template<SettingOptions options>
struct SettingTraits<Unset, options> : SettingTraitsBase<Unset>
{
    static constexpr bool has_unset{true};
};

template<SettingOptions options>
struct SettingTraits<Enabled, options> : SettingTraitsBase<Enabled>
{
    static constexpr bool has_enabled{true};
    static void Update(Enabled, setting_t& out) {}
};

template<SettingOptions options>
struct SettingTraits<Disabled, options> : SettingTraitsBase<Disabled>
{
    static constexpr bool has_disabled{true};
    static void Update(Disabled, setting_t& out) {}
};

template<SettingOptions options>
struct SettingTraits<bool, options> : SettingTraitsBase<bool>
{
    static constexpr bool has_bool = true;
    static void Update(Enabled, setting_t& out) { out = true; }
    static void Update(Disabled, setting_t& out) { out = false; }
    static void Update(bool value, setting_t& out) { out = value; }
};

template<typename T, SettingOptions options>
requires std::is_integral_v<T>
struct SettingTraits<T, options> : SettingTraitsBase<T>
{
    using setting_t = T;
    using number_t = T;
    static constexpr bool has_number = true;
    static void Update(Disabled, setting_t& out) { out = 0; }
    static void Update(const T value, setting_t& out) { out = value; }
};

template<SettingOptions options>
struct SettingTraits<std::string, options> : SettingTraitsBase<std::string>
{
    static constexpr bool has_string = true;
    static void Update(Disabled, setting_t& out) { out.clear(); }
    static void Update(std::string_view value, setting_t& out) { out = value; }
};

template<SettingOptions options>
struct SettingTraits<std::string_view, options> : SettingTraitsBase<std::string_view>
{
    static constexpr bool has_string = true;
    static void Update(Disabled, setting_t& out) { out = {}; }
    static void Update(std::string_view value, setting_t& out) { out = value; }
};

template<typename T, SettingOptions options>
struct SettingTraits<std::optional<T>, options> : SettingTraits<T, options>
{
    using setting_t = std::optional<T>;
    using wrapped_t = SettingTraits<T, options>;
    static void Update(auto&& value, setting_t& out) { if (!out) out.emplace(); wrapped_t::Update(value, *out); }
};

template<typename T, SettingOptions options>
struct SettingTraits<std::vector<T>, options> : SettingTraits<T, options>
{
    using setting_t = std::vector<T>;
    using wrapped_t = SettingTraits<T, options>;
    static constexpr bool has_list = true;
    static void Update(auto&& value, setting_t& out)
    {
        if constexpr(options.disabled_list_means_clear && std::is_same_v<std::remove_reference_t<decltype(value)>, Disabled>) {
            out.clear();
        } else {
            out.emplace_back();
            wrapped_t::Update(value, out.back());
        }
    }
};

template<SettingOptions options, int index, typename... Types>
struct VariantTraits;

template<SettingOptions options, int index, typename First, typename... Rest>
struct VariantTraits<options, index, First, Rest...>
{
    using cur_t = SettingTraits<First, options>;
    using rest_t = VariantTraits<options, index + 1, Rest...>;
    using rest_merged_t = rest_t::merged_t;
    struct merged_t {
        template<typename T, typename U>
        using nonvoid_t = std::conditional_t<!std::is_void_v<T>, T, U>;
        using number_t = nonvoid_t<typename cur_t::number_t, typename rest_merged_t::number_t>;
        static constexpr bool has_unset = cur_t::has_unset || rest_merged_t::has_unset;
        static constexpr bool has_enabled = cur_t::has_enabled || rest_merged_t::has_enabled;
        static constexpr bool has_disabled = cur_t::has_disabled || rest_merged_t::has_disabled;
        static constexpr bool has_bool = cur_t::has_bool || rest_merged_t::has_bool;
        static constexpr bool has_number = cur_t::has_number || rest_merged_t::has_number;
        static constexpr bool has_string = cur_t::has_string || rest_merged_t::has_string;
        static constexpr bool has_list = cur_t::has_list || rest_merged_t::has_list;
    };
    static constexpr int index_unset{cur_t::has_unset ? index : rest_t::index_unset};
    static constexpr int index_enabled{cur_t::has_enabled ? index : rest_t::index_enabled};
    static constexpr int index_disabled{cur_t::has_disabled ? index : rest_t::index_disabled};
    static constexpr int index_bool{cur_t::has_bool ? index : rest_t::index_bool};
    static constexpr int index_number{cur_t::has_number ? index : rest_t::index_number};
    static constexpr int index_string{cur_t::has_string ? index : rest_t::index_string};
    static constexpr int index_list{cur_t::has_list ? index : rest_t::index_list};
};

template<SettingOptions options, int index, typename Last>
struct VariantTraits<options, index, Last>
{
    using cur_t = SettingTraits<Last, options>;
    using merged_t = cur_t;
    static constexpr int index_unset{cur_t::has_unset ? index : -1};
    static constexpr int index_enabled{cur_t::has_enabled ? index : -1};
    static constexpr int index_disabled{cur_t::has_disabled ? index : -1};
    static constexpr int index_bool{cur_t::has_bool ? index : -1};
    static constexpr int index_number{cur_t::has_number ? index : -1};
    static constexpr int index_string{cur_t::has_string ? index : -1};
    static constexpr int index_list{cur_t::has_list ? index : -1};
};

template<typename... T, SettingOptions options>
struct SettingTraits<std::variant<T...>, options> : VariantTraits<options, 0, T...>::merged_t
{
    using setting_t = std::variant<T...>;
    using variant_t = VariantTraits<options, 0, T...>;
    using merged_t = variant_t::merged_t;
    static constexpr int index_value{merged_t::has_bool ? variant_t::index_bool :
                                     merged_t::has_number ? variant_t::index_number :
                                     merged_t::has_string ? variant_t::index_string : -1};

    static setting_t Construct()
    {
        constexpr int index{merged_t::has_unset ? variant_t::index_unset : 0};
        using construct_t = std::variant_alternative_t<index, setting_t>;
        return SettingTraits<construct_t, options>::Construct();
    }
    static void Update(Enabled value, setting_t& out)
    {
        constexpr int index{merged_t::has_enabled ? variant_t::index_enabled : index_value};
        UpdateVariant<index>(value, out);
    }
    static void Update(Disabled value, setting_t& out)
    {
        constexpr int index{merged_t::has_disabled ? variant_t::index_disabled :
                            merged_t::has_list ? variant_t::index_list : index_value};
        UpdateVariant<index>(value, out);
    }
    template<typename M = merged_t>
    static void Update(bool value, setting_t& out) requires (M::has_bool) { UpdateVariant<variant_t::index_bool>(value, out); }
    template<typename M = merged_t>
    static void Update(M::number_t value, setting_t& out) requires (M::has_number) { UpdateVariant<variant_t::index_number>(value, out); }
    template<typename M = merged_t>
    static void Update(std::string_view value, setting_t& out) requires (M::has_string) { UpdateVariant<variant_t::index_string>(value, out); }

    template<int index>
    static void UpdateVariant(auto&& value, setting_t& out)
    {
        using elem_t = std::variant_alternative_t<index, setting_t>;
        elem_t* elem{std::get_if<index>(out)};
        if (!elem) elem = &out.template emplace<index>();
        SettingTraits<elem_t, options>::Update(value, *elem);
    }
};

// Helper to check if a type is an instance of a template class
template <typename T, template<typename...> class Template>
struct is_instance_of : std::false_type {};

template <template<typename...> class Template, typename... Args>
struct is_instance_of<Template<Args...>, Template> : std::true_type {};

// The HasOption function to check for a template instance in a pack
template<template<typename...> class Template, typename First, typename... Rest>
constexpr bool HasOption() {
    if constexpr (is_instance_of<std::decay_t<First>, Template>::value) {
        return true;
    } else if constexpr (sizeof...(Rest) > 0) {
        return HasOption<Template, Rest...>();
    } else {
        return false;
    }
}
template<template<typename...> class Template>
constexpr bool HasOption() { return false; }

// The GetOption function to retrieve the first instance of a template class from a pack
template<template<typename...> class Template, typename First, typename... Rest>
auto& GetOption(First& first, Rest&... rest) {
    if constexpr (is_instance_of<std::decay_t<First>, Template>::value) {
        return first;
    } else {
        return GetOption<Template>(rest...);
    }
}

inline std::string_view SettingName(std::string_view summary)
{
    std::string_view name{summary};
    size_t pos{name.find_first_of("=")};
    if (pos != std::string_view::npos) {
        name = name.substr(0, pos);
    }
    return name;
}

template<SettingOptions options, typename T>
constexpr int SettingFlags()
{
    using Traits = SettingTraits<T, options>;
    int flags = 0;
    if constexpr (Traits::has_bool || Traits::has_enabled) {
        flags |= ArgsManager::ALLOW_BOOL;
    }
    if constexpr (Traits::has_number) {
        flags |= ArgsManager::ALLOW_INT;
    }
    if constexpr (Traits::has_string) {
        flags |= ArgsManager::ALLOW_STRING;
    }
    if constexpr (Traits::has_list) {
        flags |= ArgsManager::ALLOW_LIST;
    }
    if (options.disallow_negation) {
        flags |= ArgsManager::DISALLOW_NEGATION;
    }
    return flags;
}

// Convert command line and config values into intermediate JSON represention.
template<SettingOptions options, typename T>
std::optional<SettingsValue> ParseSetting(const KeyInfo& key, const std::string* value, std::string& error)
{
    auto setting{InterpretValue(key, value, SettingFlags<options, T>(), error)};
    using Traits = SettingTraits<T, options>;
    if constexpr (Traits::has_number) {
        if (int64_t parsed_int; setting && setting->isNum() && ParseInt64(setting->getValStr(), &parsed_int)) {
            using Limits = std::numeric_limits<typename Traits::number_t>;
            if (parsed_int < Limits::min() || parsed_int > Limits::max()) {
                error = strprintf("Setting %s value '%s' is out of range for type %s.", key.name, setting->write(), typeid(typename Traits::number_t).name());
                return {};
            }
        }
    }
    return setting;
}

// Convert settings.json values and intermediate JSON values into final settings value.
template<SettingOptions options, typename T>
bool GetSetting(const SettingsValue& value, T& out, std::string& error)
{
    using Traits = SettingTraits<T, options>;
    if constexpr (Traits::has_bool) {
        std::optional<bool> bool_value{ConvertToBool(value, /*typed_arg=*/true)};
        if (bool_value) {
            if (*bool_value) {
                Traits::Update(Enabled{}, out);
            } else {
                Traits::Update(Disabled{}, out);
            }
            return true;
        }
    }
    if constexpr (Traits::has_number) {
        std::optional<int64_t> int_value{ConvertToInt(value, /*typed_arg=*/true)};
        if (int_value) {
            Traits::Update(*int_value, out);
            return true;
        }
    }
    if constexpr (Traits::has_string) {
        std::optional<std::string> string_value{ConvertToString(value, /*typed_arg=*/true)};
        if (string_value) {
            Traits::Update(*string_value, out);
            return true;
        }
    }
    return true;
}

template<SettingOptions options, typename T>
void SettingRegister(auto& manager, auto summary, auto help, auto category, auto parse_fn, auto default_fn, auto&&... register_options)
{
    std::string help_str;
    if constexpr (HasOption<HelpArgs, decltype(register_options)...>()) {
        help_str = std::apply([&](auto&&... args) { return strprintf(help, args...); },
                              GetOption<HelpArgs>(register_options...).args);
    } else if constexpr (!std::is_same_v<decltype(default_fn), std::nullptr_t>) {
        help_str = strprintf(help, default_fn());
    } else {
        help_str = help;
    }

    ArgsManager::ParseFn arg_parse;
    if constexpr (HasOption<ParseArgs, decltype(register_options)...>()) {
        arg_parse = [parse_fn, parse_args = GetOption<ParseArgs>(register_options...).args] (const KeyInfo& key, const std::string* value, std::string& error) {
            return std::apply([&](auto&&... args) { return parse_fn(key, value, error, args...); }, parse_args);
        };
    } else if constexpr (!std::is_same_v<decltype(parse_fn), std::nullptr_t>) {
        arg_parse = parse_fn;
    } else {
        arg_parse = &ParseSetting<options, T>;
    }

    manager.AddArg(summary, help_str, SettingFlags<options, T>(), category, arg_parse);
}

template<SettingOptions options, typename T>
T SettingConstruct(auto default_fn)
{
    if constexpr (std::is_same_v<decltype(default_fn), std::nullptr_t>) {
        using Traits = SettingTraits<T, options>;
        return Traits::Construct();
    } else {
        return default_fn();
    }
}

template<SettingOptions options, typename T, typename ParseType>
bool SettingUpdate(auto& manager, auto summary, auto default_fn, auto get_fn, T& out, std::string& error)
{
    const std::string name{SettingName(summary)};
    using Traits = SettingTraits<T, options>;
    auto update_fn{[&](const SettingsValue& value) {
        if (options.empty_string_means_unset && value.isStr() && value.get_str().empty()) {
            if constexpr (!std::is_same_v<decltype(default_fn), std::nullptr_t>) {
                Traits::Update(default_fn(), out);
            }
            return true;
        }
        if constexpr (std::is_same_v<decltype(get_fn), std::nullptr_t>) {
            return GetSetting<options, T>(value, out, error);
        } else {
            return get_fn(value, out, error);
        }
    }};

    using ParseTraits = SettingTraits<ParseType, options>;
    if constexpr (ParseTraits::has_list) {
        for (const SettingsValue& value : manager.GetSettingList(name))
        {
            if (!update_fn(value)) return false;
        }
    } else {
        return update_fn(manager.GetSetting(name));
    }
}
} // namespace internal
} // namespace common

#endif // BITCOIN_COMMON_SETTING_INTERNAL_H
