// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_BASE_H
#define BITCOIN_INTERFACES_BASE_H

#include <cassert>
#include <memory>

namespace interfaces {

class Base;

//! Close hook.
class CloseHook
{
public:
    virtual ~CloseHook() {}

    //! Handle interface being closed or destroyed.
    virtual void onClose(Base& interface) {}

    std::unique_ptr<CloseHook> m_next_hook;
};

//! Close hook that encapsulate and deletes a moveable object.
template <typename T>
class Deleter : public CloseHook
{
public:
    explicit Deleter(T&& value) : m_value(std::move(value)) {}
    void onClose(Base& interface) override { T(std::move(m_value)); }
    T m_value;
};

//! Base class for interfaces.
class Base
{
public:
    //! Destructor, calls close hooks.
    virtual ~Base();

    // Call close hooks without waiting for destructor.
    void close();

    //! Add close hook.
    void addCloseHook(std::unique_ptr<CloseHook> close_hook);

    std::unique_ptr<CloseHook> m_close_hook;
};

} // namespace interfaces

#endif // BITCOIN_INTERFACES_BASE_H
