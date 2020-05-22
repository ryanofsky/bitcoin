// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_ASYNC_UPDATE_H
#define BITCOIN_QT_ASYNC_UPDATE_H

#include <QObject>

#include <future>

namespace internal {
//! Queue a function to run in an object's event loop. This can be
//! replaced by a call to theQMetaObject::invokeMethod functor overload after Qt
//! 5.10, but for now use a QObject::connect for compatibility with older Qt
//! versions, based on
//! https://stackoverflow.com/questions/21646467/how-to-execute-a-functor-or-a-lambda-in-a-given-thread-in-qt-gcd-style
template <typename Fn>
static void ObjectInvoke(QObject& object, Fn&& fn)
{
    QObject source;
    QObject::connect(&source, &QObject::destroyed, &object, std::forward<Fn>(fn), Qt::QueuedConnection);
}

//! Implementation of AsyncUpdate(), see description below.
template <typename AsyncFn, typename UpdateFn>
class AsyncUpdateImpl : public QObject
{
public:
    AsyncUpdateImpl(QObject& parent, AsyncFn async_fn, UpdateFn update_fn)
        : QObject(&parent), m_async_fn(std::move(async_fn)), m_update_fn(std::move(update_fn)),
          m_async(std::async(std::launch::async, [this] {
              m_result = m_async_fn();
              ObjectInvoke(*this, [this] { m_update_fn(m_result); });
          })) {}
    ~AsyncUpdateImpl() { m_async.wait(); }

    AsyncFn m_async_fn;
    UpdateFn m_update_fn;
    std::future<void> m_async;
    using Result = typename std::result_of<AsyncFn()>::type;
    Result m_result;
};
} // namespace internal

//! Call async_fn asynchronously in a new thread, then pass the return value as
//! an argument to update_fn, calling update_fn from the specified parent
//! object's QEvent loop.
template <typename AsyncFn, typename UpdateFn>
void AsyncUpdate(QObject& parent, AsyncFn async_fn, UpdateFn update_fn)
{
    new internal::AsyncUpdateImpl<AsyncFn, UpdateFn>(parent, std::move(async_fn), std::move(update_fn));
}

#endif // BITCOIN_QT_ASYNC_UPDATE_H
