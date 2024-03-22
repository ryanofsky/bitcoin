// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or https://www.opensource.org/licenses/mit-license.php.

#include <util/result.h>

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <util/translation.h>

namespace util {
namespace detail {
bilingual_str JoinMessages(const std::vector<bilingual_str>& errors, const std::vector<bilingual_str>& warnings)
{
    bilingual_str result;
    for (const auto& messages : {errors, warnings}) {
        for (const auto& message : messages) {
            if (!result.empty()) result += Untranslated(" ");
            result += message;
        }
    }
    return result;
}

template<>
void ResultTraits<Messages>::MergeInto(Messages& dst, Messages& src) {
    dest.errors.insert(dest.errors.end(), std::make_move_iterator(src.errors.begin()), std::make_move_iterator(src.errors.end()));
    dest.warnings.insert(dest.warnings.end(), std::make_move_iterator(src.warnings.begin()), std::make_move_iterator(src.warnings.end()));
    src.errors.clear();
    src.warnings.clear();
}
} // namespace detail
} // namespace util
