// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/base.h>

namespace interfaces {

Base::~Base() { close(); }

void Base::close()
{
    for (auto hook = std::move(m_close_hook); hook; hook = std::move(hook->m_next_hook)) {
        hook->onClose(*this);
    }
}

void Base::addCloseHook(std::unique_ptr<CloseHook> close_hook)
{
    assert(close_hook);
    assert(!close_hook->m_next_hook);
    close_hook->m_next_hook = std::move(m_close_hook);
    m_close_hook = std::move(close_hook);
}

} // namespace interfaces
