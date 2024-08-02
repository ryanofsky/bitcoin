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
    static constexpr bool has_empty{false};
    static constexpr bool has_enabled{false};
    static constexpr bool has_disabled{false};
    static constexpr bool has_bool{false};
    static constexpr bool has_number{false};
    static constexpr bool has_string{false};
    static constexpr bool has_list{false};
    static setting_t Construct() { return {}; }
};

template<typename T>
struct SettingTraits;

template<>
struct SettingTraits<Unset> : SettingTraitsBase<Unset>
{
    static constexpr bool has_unset{true};
    static void Update(Unset, setting_t& out) {}
};

template<>
struct SettingTraits<Empty> : SettingTraitsBase<Empty>
{
    static constexpr bool has_empty{true};
    static void Update(Empty, setting_t& out) {}
};

template<>
struct SettingTraits<Enabled> : SettingTraitsBase<Enabled>
{
    static constexpr bool has_enabled{true};
    static void Update(Enabled, setting_t& out) {}
};

template<>
struct SettingTraits<Disabled> : SettingTraitsBase<Disabled>
{
    static constexpr bool has_disabled{true};
    static void Update(Disabled, setting_t& out) {}
};

template<>
struct SettingTraits<bool> : SettingTraitsBase<bool>
{
    static constexpr bool has_bool = true;
    static void Update(Empty, setting_t& out) {}
    static void Update(Enabled, setting_t& out) { out = true; }
    static void Update(Disabled, setting_t& out) { out = false; }
    static void Update(bool value, setting_t& out) { out = value; }
};

template<typename T>
requires std::is_integral_v<T>
struct SettingTraits<T> : SettingTraitsBase<T>
{
    using setting_t = T;
    using number_t = T;
    static constexpr bool has_number = true;
    static void Update(Empty, setting_t& out) {}
    static void Update(Disabled, setting_t& out) { out = 0; }
    static void Update(const T value, setting_t& out) { out = value; }
};

template<>
struct SettingTraits<std::string> : SettingTraitsBase<std::string>
{
    static constexpr bool has_string = true;
    static void Update(Empty, setting_t& out) {}
    static void Update(Disabled, setting_t& out) { out.clear(); }
    static void Update(std::string_view value, setting_t& out) { out = value; }
};

template<>
struct SettingTraits<std::string_view> : SettingTraitsBase<std::string_view>
{
    static constexpr bool has_string = true;
    static void Update(Empty, setting_t& out) {}
    static void Update(Disabled, setting_t& out) { out = {}; }
    static void Update(std::string_view value, setting_t& out) { out = value; }
};

template<typename T>
struct SettingTraits<std::optional<T>> : SettingTraits<T>
{
    using setting_t = std::optional<T>;
    using wrapped_t = SettingTraits<T>;
    static void Update(Empty, setting_t& out) { }
    static void Update(auto&& value, setting_t& out) { if (!out) out.emplace(); wrapped_t::Update(value, *out); }
};

template<typename T>
struct SettingTraits<std::vector<T>> : SettingTraits<T>
{
    using setting_t = std::vector<T>;
    using wrapped_t = SettingTraits<T>;
    static constexpr bool has_list = true;
    static void Update(Disabled, setting_t& out) { out.clear(); }
    static void Update(auto&& value, setting_t& out) { out.emplace_back(); wrapped_t::Update(value, out.back()); }
};

template<typename Traits, size_t Index, typename... Types>
struct VariantTraits;

template<int Index, typename Type>
struct VariantTraits<void, Index, Type>
{
    using merged_t = SettingTraits<Type>;
    static constexpr int index_unset{Index};
    static constexpr int index_empty{Index};
    static constexpr int index_enabled{Index};
    static constexpr int index_disabled{Index};
    static constexpr int index_bool{Index};
    static constexpr int index_number{Index};
    static constexpr int index_string{Index};
    static constexpr int index_list{Index};
};

template<typename Traits, int Index, typename Type>
struct VariantTraits<Traits, Index, Type>
{
    using cur_t = Traits::merged_t;
    struct merged_t {
        template<typename T, typename U>
        using nonvoid_t = std::conditional_t<!std::is_void_v<T>, T, U>;
        using number_t = nonvoid_t<typename cur_t::number_t, typename Type::number_t>;
        static constexpr bool has_unset = cur_t::has_empty || Type::has_empty;
        static constexpr bool has_empty = cur_t::has_empty || Type::has_empty;
        static constexpr bool has_enabled = cur_t::has_enabled || Type::has_enabled;
        static constexpr bool has_disabled = cur_t::has_disabled || Type::has_disabled;
        static constexpr bool has_bool = cur_t::has_bool || Type::has_bool;
        static constexpr bool has_number = cur_t::has_number || Type::has_number;
        static constexpr bool has_string = cur_t::has_string || Type::has_string;
        static constexpr bool has_list = cur_t::has_list || Type::has_list;
    };
    static constexpr int index_unset{cur_t::has_empty ? Traits::index_empty : Index};
    static constexpr int index_empty{cur_t::has_empty ? Traits::index_empty : Index};
    static constexpr int index_enabled{cur_t::has_enabled ? Traits::index_enabled : Index};
    static constexpr int index_disabled{cur_t::has_disabled ? Traits::index_disabled : Index};
    static constexpr int index_bool{cur_t::has_bool ? Traits::index_bool : Index};
    static constexpr int index_number{cur_t::has_number ? Traits::index_number : Index};
    static constexpr int index_string{cur_t::has_string ? Traits::index_string : Index};
    static constexpr int index_list{cur_t::has_list ? Traits::index_list : Index};
    static constexpr int index_value{merged_t::has_bool ? index_bool :
                                     merged_t::has_number ? index_number :
                                     merged_t::has_string ? index_string : -1};
};

template<typename Traits, int Index, typename Type1, typename Type2, typename... Rest>
struct VariantTraits<Traits, Index, Type1, Type2, Rest...> : VariantTraits<VariantTraits<Traits, Index, Type1>, Index + 1, Type2, Rest...> {
};

template<typename... T>
struct SettingTraits<std::variant<T...>> : VariantTraits<void, 0, T...>::merged_t
{
    using setting_t = std::variant<T...>;
    using variant_t = VariantTraits<void, 0, T...>;
    using merged_t = variant_t::merged_t;
    static setting_t Construct()
    {
        constexpr int index{merged_t::has_unset ? variant_t::index_unset : 0};
        using construct_t = std::variant_alternative_t<index, setting_t>;
        return SettingTraits<construct_t>::Construct();
    }
    static void Update(Empty value, setting_t& out)
    {
        constexpr int index{merged_t::has_empty ? variant_t::index_empty :
                            merged_t::has_unset ? variant_t::index_unset : 0};
        UpdateVariant<index>(value, out);
    }
    static void Update(Enabled value, setting_t& out)
    {
        constexpr int index{merged_t::has_enabled ? variant_t::index_enabled : variant_t::index_value};
        UpdateVariant<index>(value, out);
    }
    static void Update(Disabled value, setting_t& out)
    {
        constexpr int index{merged_t::has_disabled ? variant_t::index_disabled :
                            merged_t::has_list ? variant_t::index_list : variant_t::index_value};
        UpdateVariant<index>(value, out);
    }
    static void Update(bool value, setting_t& out) { UpdateVariant<variant_t::index_bool>(value, out); }
    static void Update(merged_t::number_t value, setting_t& out) { UpdateVariant<variant_t::index_number>(value, out); }
    static void Update(std::string_view value, setting_t& out) { UpdateVariant<variant_t::index_string>(value, out); }

    template<int index>
    static void UpdateVariant(auto&& value, setting_t& out)
    {
        using elem_t = std::variant_alternative_t<index, setting_t>;
        elem_t* elem{std::get_if<index>(out)};
        if (!elem) elem = &out.template emplace<index>();
        SettingTraits<elem_t>::Update(value, *elem);
    }
};

template<typename T, typename DefaultFn>
void SettingUpdateSingle(const SettingsValue& value, DefaultFn& default_fn, T& out)
{
    using Traits = SettingTraits<T>;
    if (value.isStr() && value.get_str().empty()) {
        if constexpr (std::is_same_v<DefaultFn, std::nullptr_t>) {
            Traits::Update(Empty{}, out);
        } else {
            Traits::Update(default_fn(), out);
        }
        return;
    }
    if constexpr (Traits::has_bool) {
        std::optional<bool> bool_value{ConvertToBool(value, /*typed_arg=*/true)};
        if (bool_value) {
            if (*bool_value) {
                Traits::Update(Enabled{}, out);
            } else {
                Traits::Update(Disabled{}, out);
            }
            return;
        }
    }
    if constexpr (Traits::has_number) {
        std::optional<int64_t> int_value{ConvertToInt(value, /*typed_arg=*/true)};
        if (int_value) {
            Traits::Update(*int_value, out);
            return;
        }
    }
    if constexpr (Traits::has_string) {
        std::optional<std::string> string_value{ConvertToString(value, /*typed_arg=*/true)};
        if (string_value) {
            Traits::Update(*string_value, out);
            return;
        }
    }
}

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

template<typename T>
void SettingRegister(auto& manager, auto summary, auto help, auto category, auto options, auto check_fn, auto default_fn, auto&&... register_options)
{
    using Traits = SettingTraits<T>;
    std::string help_str;

    if constexpr (HasOption<HelpArgs, decltype(register_options)...>()) {
        help_str = std::apply([&](auto&&... args) { return strprintf(help, args...); },
                              GetOption<HelpArgs>(register_options...).args);
    } else if constexpr (!std::is_same_v<decltype(default_fn), std::nullptr_t>) {
        help_str = strprintf(help, default_fn());
    } else {
        help_str = help;
    }

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

    std::function<bool(common::SettingsValue value, std::string& error)> checks;
    if constexpr (!std::is_same_v<decltype(check_fn), std::nullptr_t>) {
        if constexpr (HasOption<CheckArgs>(register_options...)) {
            checks = [next_check = std::move(check_fn), bound_args = GetOption<CheckArgs>(register_options...)](auto&&... args) -> bool {
                return std::apply([&](auto&&... tuple_args){ return next_check(tuple_args..., args...); }, bound_args);
            };
        } else {
            checks = check_fn;
        }
    }

    if constexpr (Traits::has_number) {
        checks = [next_check=std::move(checks), arg_name=SettingName(summary)](std::string_view value, std::string& error) -> bool {
            int64_t parsed_int;
            if (ParseInt64(value, &parsed_int)) {
                using Limits = std::numeric_limits<typename Traits::number_t>;
                if (value < Limits::min() || value > Limits::max()) {
                    error = strprintf("Setting %s value '%s' is out of range for type %s.", arg_name, value, typeid(Traits::number_t).name());
                    return false;
                }
            }
            return next_check ? next_check(value, error) : true;
        };
    }

    manager.AddArg(summary, help_str, flags, category, checks);
}

template<typename T>
T SettingConstruct(auto default_fn)
{
    if constexpr (std::is_same_v<decltype(default_fn), std::nullptr_t>) {
        using Traits = SettingTraits<T>;
        return Traits::Construct();
    } else {
        return default_fn();
    }
}

template<typename T>
void SettingUpdate(auto& manager, auto summary, auto default_fn, T& out)
{
    using Traits = SettingTraits<T>;
    if constexpr (Traits::has_list) {
        for (const SettingsValue& value : manager.GetSettingList(std::string{SettingName(summary)}))
        {
            SettingUpdateSingle(value, default_fn, out);
        }
    } else {
        const SettingsValue& value{manager.GetSetting(std::string{SettingName(summary)})};
        SettingUpdateSingle(value, default_fn, out);
    }
}
} // namespace internal
} // namespace common

#endif // BITCOIN_COMMON_SETTING_INTERNAL_H
