// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_SCOPE_H
#define BITCOIN_UTIL_SCOPE_H

namespace util {
//! Minimal alternative to std::experimental::scope_exit
//! https://en.cppreference.com/w/cpp/experimental/scope_exit
//! http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0052r9.pdf
template<typename F>
class ScopeExit
{
public:
    explicit ScopeExit(F&& f) : m_f(std::move(f)) {}
    ScopeExit(ScopeExit&& other) : m_f(std::move(other.m_f)) {}
    ~ScopeExit() { m_f(); }
    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;
    ScopeExit& operator=(ScopeExit&&) = delete;
private:
    F m_f;
};

template<typename F>
ScopeExit<typename std::remove_reference<F>::type> MakeScopeExit(F&& f)
{
    return ScopeExit<typename std::remove_reference<F>::type>(std::forward<F>(f));
}
} // namespace util

#endif // BITCOIN_UTIL_SCOPE_H
