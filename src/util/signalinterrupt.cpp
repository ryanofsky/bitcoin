// Copyright (c) 2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <util/signalinterrupt.h>

#ifdef WIN32
#include <mutex>
#else
#include <util/tokenpipe.h>
#endif

#include <ios>
#include <optional>

namespace util {

bool SignalInterrupt::open()
{
#ifndef WIN32
    if (!m_pipe_r.IsOpen() || !m_pipe_w.IsOpen()) {
        std::optional<TokenPipe> pipe = TokenPipe::Make();
        if (!pipe) return false;
        m_pipe_r = pipe->TakeReadEnd();
        m_pipe_w = pipe->TakeWriteEnd();
    }
#endif
    return true;
}

SignalInterrupt::operator bool() const
{
    return m_flag;
}

bool SignalInterrupt::reset()
{
    // Cancel existing interrupt by waiting for it, this will reset condition flags and remove
    // the token from the pipe.
    if (*this && !wait()) return false;
    m_flag = false;
    return true;
}

bool SignalInterrupt::operator()()
{
    if (!open()) return false;
#ifdef WIN32
    std::unique_lock<std::mutex> lk(m_mutex);
    m_flag = true;
    m_cv.notify_one();
#else
    // This must be reentrant and safe for calling in a signal handler, so using a condition variable is not safe.
    // Make sure that the token is only written once even if multiple threads call this concurrently or in
    // case of a reentrant signal.
    if (!m_flag.exchange(true)) {
        // Write an arbitrary byte to the write end of the pipe.
        int res = m_pipe_w.TokenWrite('x');
        if (res != 0) {
            return false;
        }
    }
#endif
    return true;
}

bool SignalInterrupt::wait()
{
    if (!open()) return false;
#ifdef WIN32
    std::unique_lock<std::mutex> lk(m_mutex);
    m_cv.wait(lk, [this] { return m_flag.load(); });
#else
    int res = m_pipe_r.TokenRead();
    if (res != 'x') {
        return false;
    }
#endif
    return true;
}

} // namespace util
