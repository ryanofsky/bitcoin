// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MP_PROXY_TYPE_STOP_TOKEN_H
#define MP_PROXY_TYPE_STOP_TOKEN_H

#include <mp/proxy-io.h>
#include <mp/util.h>

#include <kj/async.h>
#include <kj/exception.h>
#include <memory>
#include <stop_token>
#include <utility>

namespace mp {
//! Client-side state of an IPC call made with a std::stop_token argument,
//! declared with `$Proxy.extraParam` in the schema. Kept by the
//! ClientParam object for the duration of the call.
//!
//! When stop is requested on the token, the request is canceled with
//! kj::Canceler. Stop can be requested from any thread before, during, or
//! after the call. If it is requested before the request is sent, the request
//! is canceled as soon as it is sent. If it is requested after the call
//! returns, it has no effect, because the stop_callback is destroyed before
//! the call returns.
class StopTokenClientState
{
public:
    StopTokenClientState(EventLoop& loop, std::stop_token token)
        : m_loop{loop}, m_callback{std::move(token), Stop{this}} {}

    //! Called on the event loop thread with the canceler wrapping the request
    //! after it is sent, and with null after the request finishes.
    void setCanceler(kj::Canceler* canceler)
    {
        const Lock lock{m_mutex};
        m_canceler = canceler;
        if (m_canceler && m_canceled) m_canceler->cancel("IPC call canceled by std::stop_token");
    }

    //! Called on the event loop thread if the request fails. Throw
    //! InterruptException if it failed because it was canceled.
    void handleError(const kj::Exception&)
    {
        const Lock lock{m_mutex};
        if (m_canceled) throw InterruptException{"IPC call canceled by std::stop_token."};
    }

private:
    //! Called by std::stop_callback on the thread requesting stop.
    void cancel()
    {
        {
            const Lock lock{m_mutex};
            m_canceled = true;
            if (!m_canceler) return;
        }
        // kj::Canceler is only safe to use on the event loop thread. Check
        // m_canceler again there because the request may have finished in the
        // meantime.
        m_loop.sync([&] {
            const Lock lock{m_mutex};
            if (m_canceler) m_canceler->cancel("IPC call canceled by std::stop_token");
        });
    }

    struct Stop
    {
        StopTokenClientState* state;
        void operator()() const { state->cancel(); }
    };

    EventLoop& m_loop;
    Mutex m_mutex;
    bool m_canceled MP_GUARDED_BY(m_mutex){false};
    kj::Canceler* m_canceler MP_GUARDED_BY(m_mutex){nullptr};
    //! Declared last so it is destroyed first, which unregisters the callback,
    //! waiting for it to finish if stop is being requested on another thread,
    //! before the members it uses are destroyed.
    std::stop_callback<Stop> m_callback;
};

//! Overload CustomMakeParamState to make IPC calls with a std::stop_token
//! argument cancelable. Calls with a token that can't be stopped (for example
//! a default-constructed one) are made normally without a canceler.
inline std::unique_ptr<StopTokenClientState> CustomMakeParamState(TypeList<std::stop_token>, Priority<1>,
                                                                  ClientInvokeContext& invoke_context,
                                                                  const std::stop_token& token)
{
    if (!token.stop_possible()) return nullptr;
    return std::make_unique<StopTokenClientState>(*invoke_context.connection.m_loop, token);
}

//! Overload CustomReadExtraParam to pass a std::stop_token to server methods.
//! Stop is requested on the token if the request is canceled while the method
//! is executing, because the client canceled it, dropped the response promise,
//! or disconnected. Cap'n Proto only reports dropped promises for methods with
//! the `$Cxx.allowCancellation` annotation.
//!
//! Stop callbacks registered with std::stop_callback run on the event loop
//! thread, so they should return quickly and not make IPC calls. As with any
//! std::stop_callback, destroying one waits for its callback to finish if it is
//! running on another thread, so methods should not destroy a stop_callback
//! while holding a mutex its callback locks.
//!
//! Methods that execute on the event loop thread (methods without a
//! `Proxy.Context` parameter) can't be canceled while executing and receive a
//! token that is never stopped.
template <typename ServerContext>
std::stop_token CustomReadExtraParam(TypeList<std::stop_token>, Priority<1>, ServerContext& server_context)
{
    // This runs with request_lock held, before the method is called, so
    // request_canceled and cancel_fn can be accessed without other locking.
    if (!server_context.request_lock) return {};
    std::stop_source source;
    if (server_context.request_canceled) source.request_stop();
    server_context.cancel_fn = [source]() mutable { source.request_stop(); };
    return source.get_token();
}
} // namespace mp

#endif // MP_PROXY_TYPE_STOP_TOKEN_H
