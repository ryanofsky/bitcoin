// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_UTIL_MEMORY_H
#define BITCOIN_UTIL_MEMORY_H

namespace util {
//! Destroy and reconstruct an object in place
//! https://stackoverflow.com/questions/2166099/calling-a-constructor-to-re-initialize-object
template <class T, typename... Args>
void Reconstruct(T& x, Args&&... args)
{
    static_assert(!std::has_virtual_destructor<T>::value, "Unsafe");
    x.~T();
    new (&x) T(std::forward<Args>(args)...);
}
} // namespace util

#endif
