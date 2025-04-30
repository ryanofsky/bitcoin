// Copyright (c) 2021 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>
#include <ipc/capnp/context.h>
#include <ipc/capnp/init.capnp.h>
#include <ipc/capnp/init.capnp.proxy.h>
#include <ipc/capnp/protocol.h>
#include <ipc/exception.h>
#include <ipc/protocol.h>
#include <kj/async.h>
#include <logging.h>
#include <mp/proxy-io.h>
#include <mp/proxy-types.h>
#include <mp/util.h>
#include <util/threadnames.h>

#include <cassert>
#include <cerrno>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <system_error>
#include <thread>

#ifndef WIN32
#include <sys/socket.h>
#endif

namespace ipc {
namespace capnp {
namespace {
void IpcLogFn(bool raise, std::string message)
{
    LogDebug(BCLog::IPC, "%s\n", message);
    if (raise) throw Exception(message);
}

class CapnpProtocol : public Protocol
{
public:
    CapnpProtocol(const char* exe_name) : m_exe_name{exe_name} {}
    ~CapnpProtocol() noexcept(true)
    {
        m_loop_ref.reset();
        if (m_loop_thread.joinable()) m_loop_thread.join();
        assert(!m_loop);
    };
    std::unique_ptr<interfaces::Init> connect(mp::Stream stream) override
    {
        startLoop();
        return mp::ConnectStream<messages::Init>(*m_loop, std::move(stream));
    }
    void listen(mp::SocketId listen_fd, interfaces::Init& init) override
    {
        startLoop();
        if (::listen(listen_fd, /*backlog=*/5) != 0) {
            throw std::system_error(errno, std::system_category());
        }
        mp::ListenConnections<messages::Init>(*m_loop, listen_fd, init);
    }
    void serve(interfaces::Init& init, const std::function<mp::Stream()>& make_stream) override
    {
        assert(!m_loop);
<<<<<<< HEAD
        mp::g_thread_context.thread_name = mp::ThreadName(exe_name);
        m_loop.emplace(exe_name, &IpcLogFn, &m_context);
        if (ready_fn) ready_fn();
        mp::ServeStream<messages::Init>(*m_loop, fd, init);
        m_parent_connection = &m_loop->m_incoming_connections.back();
||||||| parent of 13aef5ad7669 (ipc: add windows support)
        mp::g_thread_context.thread_name = mp::ThreadName(exe_name);
        m_loop.emplace(exe_name, &IpcLogFn, &m_context);
        if (ready_fn) ready_fn();
        mp::ServeStream<messages::Init>(*m_loop, fd, init);
=======
        mp::g_thread_context.thread_name = mp::ThreadName(m_exe_name);
        m_loop.emplace(m_exe_name, &IpcLogFn, &m_context);
        mp::ServeStream<messages::Init>(*m_loop, make_stream(), init);
>>>>>>> 13aef5ad7669 (ipc: add windows support)
        m_loop->loop();
        m_loop.reset();
    }
<<<<<<< HEAD
    void disconnectIncoming() override
    {
        if (!m_loop) return;
        // Delete incoming connections, except the connection to a parent
        // process (if there is one), since a parent process should be able to
        // monitor and control this process, even during shutdown.
        m_loop->sync([&] {
            m_loop->m_incoming_connections.remove_if([this](mp::Connection& c) { return &c != m_parent_connection; });
        });
    }
||||||| parent of 13aef5ad7669 (ipc: add windows support)
=======
    mp::Stream makeStream(mp::SocketId socket) override
    {
        startLoop();
        return m_loop->m_io_context.lowLevelProvider->wrapSocketFd(socket, kj::LowLevelAsyncIoProvider::TAKE_OWNERSHIP);
    }
>>>>>>> 13aef5ad7669 (ipc: add windows support)
    void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) override
    {
        mp::ProxyTypeRegister::types().at(type)(iface).cleanup_fns.emplace_back(std::move(cleanup));
    }
    Context& context() override { return m_context; }
    void startLoop()
    {
        if (m_loop) return;
        std::promise<void> promise;
        m_loop_thread = std::thread([&] {
            util::ThreadRename("capnp-loop");
<<<<<<< HEAD
            m_loop.emplace(exe_name, &IpcLogFn, &m_context);
            m_loop_ref.emplace(*m_loop);
||||||| parent of 13aef5ad7669 (ipc: add windows support)
            m_loop.emplace(exe_name, &IpcLogFn, &m_context);
            {
                std::unique_lock<std::mutex> lock(m_loop->m_mutex);
                m_loop->addClient(lock);
            }
=======
            m_loop.emplace(m_exe_name, &IpcLogFn, &m_context);
            {
                std::unique_lock<std::mutex> lock(m_loop->m_mutex);
                m_loop->addClient(lock);
            }
>>>>>>> 13aef5ad7669 (ipc: add windows support)
            promise.set_value();
            m_loop->loop();
            m_loop.reset();
        });
        promise.get_future().wait();
    }
    const char* m_exe_name;
    Context m_context;
<<<<<<< HEAD
    std::thread m_loop_thread;
    //! EventLoop object which manages I/O events for all connections.
||||||| parent of 13aef5ad7669 (ipc: add windows support)
    std::thread m_loop_thread;
=======
>>>>>>> 13aef5ad7669 (ipc: add windows support)
    std::optional<mp::EventLoop> m_loop;
<<<<<<< HEAD
    //! Reference to the same EventLoop. Increments the loop’s refcount on
    //! creation, decrements on destruction. The loop thread exits when the
    //! refcount reaches 0. Other IPC objects also hold their own EventLoopRef.
    std::optional<mp::EventLoopRef> m_loop_ref;
    //! Connection to parent, if this is a child process spawned by a parent process.
    mp::Connection* m_parent_connection{nullptr};
||||||| parent of 13aef5ad7669 (ipc: add windows support)
=======
    std::thread m_loop_thread;
>>>>>>> 13aef5ad7669 (ipc: add windows support)
};
} // namespace

std::unique_ptr<Protocol> MakeCapnpProtocol(const char* exe_name) { return std::make_unique<CapnpProtocol>(exe_name); }
} // namespace capnp
} // namespace ipc
