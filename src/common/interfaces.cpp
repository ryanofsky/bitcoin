// Copyright (c) 2021-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <btcsignals.h>
#include <interfaces/echo.h>
#include <interfaces/handler.h>

#include <memory>
#include <utility>

namespace common {
namespace {
class CleanupHandler : public interfaces::Handler
{
public:
    explicit CleanupHandler(std::function<void()> cleanup) : m_cleanup(std::move(cleanup)) {}
    ~CleanupHandler() override { if (!m_cleanup) return; m_cleanup(); m_cleanup = nullptr; }
    void disconnect() override { if (!m_cleanup) return; m_cleanup(); m_cleanup = nullptr; }
    bool connected() override { return bool{m_cleanup}; }
    std::function<void()> m_cleanup;
};

class SignalHandler : public interfaces::Handler
{
public:
<<<<<<< HEAD
    explicit SignalHandler(btcsignals::connection connection) : m_connection(std::move(connection)) {}

||||||| parent of 9c554d63c7f (indexes: Move sync thread from index to node)
    explicit SignalHandler(boost::signals2::connection connection) : m_connection(std::move(connection)) {}

=======
    explicit SignalHandler(boost::signals2::connection connection) : m_connection(std::move(connection)) {}
>>>>>>> 9c554d63c7f (indexes: Move sync thread from index to node)
    void disconnect() override { m_connection.disconnect(); }
<<<<<<< HEAD

    btcsignals::scoped_connection m_connection;
||||||| parent of 9c554d63c7f (indexes: Move sync thread from index to node)

    boost::signals2::scoped_connection m_connection;
=======
    bool connected() override { return m_connection.connected(); }
    boost::signals2::scoped_connection m_connection;
>>>>>>> 9c554d63c7f (indexes: Move sync thread from index to node)
};

class EchoImpl : public interfaces::Echo
{
public:
    std::string echo(const std::string& echo) override { return echo; }
};
} // namespace
} // namespace common

namespace interfaces {
std::unique_ptr<Handler> MakeCleanupHandler(std::function<void()> cleanup)
{
    return std::make_unique<common::CleanupHandler>(std::move(cleanup));
}

std::unique_ptr<Handler> MakeSignalHandler(btcsignals::connection connection)
{
    return std::make_unique<common::SignalHandler>(std::move(connection));
}

std::unique_ptr<Echo> MakeEcho() { return std::make_unique<common::EchoImpl>(); }
} // namespace interfaces
