// Copyright (c) The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <kj/async.h>
#include <kj/common.h>
#include <kj/debug.h>
#include <kj/memory.h>
#include <kj/test.h>
#include <mp/proxy.h>
#include <mp/proxy-io.h>
#include <mp/util.h>
#include <mp/test/foo.capnp.h>
#include <mp/test/foo.capnp.proxy.h>
#include <mp/test/socketlistener.h>

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstring> // IWYU pragma: keep
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

#ifndef WIN32
#include <sys/socket.h>
#endif

namespace mp {
namespace test {
namespace {

constexpr auto FAILURE_TIMEOUT = std::chrono::seconds{30};

//! Default event loop log handler used by tests, throws so the calling code
//! can assert on errors.
void DefaultLogHandler(mp::LogMessage log)
{
    if (log.level == mp::Log::Raise)
        throw std::runtime_error(log.message);
}

class TestSetup
{
public:
    mp::EventLoop* loop;
    std::optional<mp::EventLoopRef> loop_ref;
    //! Thread variable should be after other struct members so the thread does
    //! not start until the other members are initialized.
    std::thread loop_thread;

    TestSetup(mp::LogFn log_handler = DefaultLogHandler)
    {
        std::promise<mp::EventLoop*> loop_promise;
        loop_thread = std::thread([&, log_handler] {
            mp::EventLoop loop("mptest-connect", log_handler);
            loop_promise.set_value(&loop);
            loop.loop();
        });
        loop = loop_promise.get_future().get();
        loop_ref.emplace(*loop);
    }

    ~TestSetup()
    {
        loop_ref.reset();
        loop_thread.join();
    }
};

KJ_TEST("ConnectStream connects to a socket serving a valid init interface")
{
    TestSetup setup;
    auto [client_fd, server_fd] = SocketPair();

    std::thread server_thread([&]() {
        mp::EventLoop server_loop("mptest-valid-server", DefaultLogHandler);
        std::unique_ptr<FooInit> init = std::make_unique<FooInit>();
        ServeStream<messages::FooInit>(server_loop, MakeStream(server_loop, server_fd), *init);
        server_loop.loop();
    });

    auto init = ConnectStream<messages::FooInit>(*setup.loop, MakeStream(*setup.loop, client_fd));

    init.reset();
    server_thread.join();
    KJ_EXPECT(true);
}

KJ_TEST("ConnectStream throws when the socket is already disconnected")
{
    TestSetup setup;
    auto [client_fd, server_fd] = SocketPair();

    CloseSocket(server_fd);

    try {
        auto init = ConnectStream<messages::FooInit>(*setup.loop, MakeStream(*setup.loop, client_fd));

        KJ_EXPECT(false);
    } catch (const std::runtime_error& e) {
        std::string_view reason = e.what();

        KJ_EXPECT(reason == "IPC client method call interrupted by disconnect.");
    }
}

KJ_TEST("ConnectStream defers disconnect failure to the first IPC request for interfaces without construct()")
{
    TestSetup setup;
    auto [client_fd, server_fd] = SocketPair();

    CloseSocket(server_fd);

    // Without a construct() method no IPC call is made during client
    // creation, so ConnectStream succeeds even though the peer is gone.
    auto foo = ConnectStream<messages::FooInterface>(*setup.loop, MakeStream(*setup.loop, client_fd));

    try {
        foo->add(1, 2);
        KJ_EXPECT(false);
    } catch (const std::runtime_error& e) {
        std::string_view reason = e.what();

        KJ_EXPECT(reason == "IPC client method called after disconnect.");
    }
}

KJ_TEST("ConnectStream handles a disconnect when no client calls are made")
{
    std::mutex mutex;
    std::condition_variable cv;
    bool warned = false;

    TestSetup setup([&](mp::LogMessage log) {
        if (log.level == mp::Log::Warning && log.message.find("unexpected network disconnect") != std::string::npos) {
            const std::lock_guard<std::mutex> lock(mutex);
            warned = true;
            cv.notify_all();
        }
        DefaultLogHandler(log);
    });
    auto [client_fd, server_fd] = SocketPair();

    CloseSocket(server_fd);

    auto foo = ConnectStream<messages::FooInterface>(*setup.loop, MakeStream(*setup.loop, client_fd));

    // The disconnect handler registered by ProxyClientBase should run and
    // delete the connection even when no calls are ever made.
    std::unique_lock<std::mutex> lock(mutex);
    KJ_EXPECT(cv.wait_for(lock, FAILURE_TIMEOUT, [&] { return warned; }));
}

KJ_TEST("ConnectStream throws when the socket disconnects after receiving data")
{
    TestSetup setup;
    auto [client_fd, server_fd] = SocketPair();

    std::thread server_thread([&]() {
        char buf[128];

        int bytes_received =
            recv(server_fd, buf, sizeof(buf), 0);

        if (bytes_received > 0) {
            CloseSocket(server_fd);
        }
    });

    try {
        auto init = ConnectStream<messages::FooInit>(*setup.loop, MakeStream(*setup.loop, client_fd));

        if (server_thread.joinable()) server_thread.join();
        KJ_EXPECT(false);
    } catch (const std::runtime_error& e) {
        if (server_thread.joinable()) server_thread.join();

        std::string_view reason = e.what();
        KJ_EXPECT(reason == "IPC client method call interrupted by disconnect.");
    }
}

KJ_TEST("ConnectStream throws when a connection accepted from a listener disconnects after receiving data")
{
    SocketListener listener;
    TestSetup setup;
    SocketId client_fd = listener.MakeConnectedSocket();
    SocketId server_fd = listener.release();

    std::thread server_thread([&]() {
        char buf[128];

        SocketId connection_fd = accept(server_fd, nullptr, nullptr);

        if (connection_fd != SocketError) {
            int bytes_received =
                recv(connection_fd, buf, sizeof(buf), 0);

            if (bytes_received > 0) {
                CloseSocket(connection_fd);
            }
        }
        CloseSocket(server_fd);
    });

    try {
        auto init = ConnectStream<messages::FooInit>(*setup.loop, MakeStream(*setup.loop, client_fd));

        if (server_thread.joinable()) server_thread.join();
        KJ_EXPECT(false);
    } catch (const std::runtime_error& e) {
        if (server_thread.joinable()) server_thread.join();

        std::string_view reason = e.what();
        KJ_EXPECT(reason == "IPC client method call interrupted by disconnect.");
    }
}

} // namespace
} // namespace test
} // namespace mp
