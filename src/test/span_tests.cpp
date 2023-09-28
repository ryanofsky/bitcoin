// Copyright (c) 2023 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <span.h>

#include <boost/test/unit_test.hpp>
#include <set>
#include <vector>

namespace {
template<typename T>
bool Spannable(T&& value, decltype(Span{value})* enable_value = nullptr)
{
    return true;
}
bool Spannable(...)
{
    return false;
}
struct SpannableYes
{
    int* data();
    size_t size();
};
struct SpannableNo
{
    void* data();
    size_t size();
};
} // namespace

BOOST_AUTO_TEST_SUITE(span_tests)

// Make sure template de
BOOST_AUTO_TEST_CASE(span_constructor_sfinae)
{
    BOOST_CHECK(Spannable(std::vector<int>{}));
    BOOST_CHECK(!Spannable(std::set<int>{}));
    BOOST_CHECK(!Spannable(std::vector<bool>{}));
    BOOST_CHECK(Spannable(std::array<int, 3>{}));
    BOOST_CHECK(Spannable(Span<int>{}));
    BOOST_CHECK(Spannable("char array"));
    BOOST_CHECK(Spannable(SpannableYes{}));
    BOOST_CHECK(!Spannable(SpannableNo{}));
}

BOOST_AUTO_TEST_SUITE_END()
