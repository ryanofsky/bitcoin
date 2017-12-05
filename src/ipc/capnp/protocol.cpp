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
#include <util/memory.h>
#include <util/threadnames.h>

#include <assert.h>
#include <errno.h>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace ipc {
namespace capnp {
namespace {
void IpcLogFn(bool raise, std::string message)
{
    LogPrint(BCLog::IPC, "%s\n", message);
    if (raise) throw Exception(message);
}

class CapnpProtocol : public Protocol
{
public:
<<<<<<< HEAD
||||||| merged common ancestors
    CapnpProtocol(const char* exe_name, interfaces::Init& init) : m_exe_name(exe_name), m_init(init) {}
=======
    CapnpProtocol(const char* exe_name, interfaces::Init& init) : m_exe_name(exe_name), m_context(init) {}
>>>>>>> Multiprocess bitcoin
    ~CapnpProtocol() noexcept(true)
    {
        if (m_loop) {
            std::unique_lock<std::mutex> lock(m_loop->m_mutex);
            m_loop->removeClient(lock);
        }
        if (m_loop_thread.joinable()) m_loop_thread.join();
        assert(!m_loop);
    };
    std::unique_ptr<interfaces::Init> connect(int fd, const char* exe_name) override
    {
        startLoop(exe_name);
        return mp::ConnectStream<messages::Init>(*m_loop, fd);
    }
    void serve(int fd, const char* exe_name, interfaces::Init& init) override
    {
        assert(!m_loop);
<<<<<<< HEAD
        mp::g_thread_context.thread_name = mp::ThreadName(exe_name);
        m_loop.emplace(exe_name, &IpcLogFn, nullptr);
        mp::ServeStream<messages::Init>(*m_loop, fd, init);
||||||| merged common ancestors
        mp::g_thread_context.thread_name = mp::ThreadName(m_exe_name);
        m_loop.emplace(m_exe_name, &IpcLogFn, nullptr);
        mp::ServeStream<messages::Init>(*m_loop, fd, m_init);
=======
        mp::g_thread_context.thread_name = mp::ThreadName(m_exe_name);
        m_loop.emplace(m_exe_name, &IpcLogFn, &m_context);
        mp::ServeStream<messages::Init>(*m_loop, fd, m_context.init);
>>>>>>> Multiprocess bitcoin
        m_loop->loop();
        m_loop.reset();
    }
    void addCleanup(std::type_index type, void* iface, std::function<void()> cleanup) override
    {
        mp::ProxyTypeRegister::types().at(type)(iface).cleanup.emplace_back(std::move(cleanup));
    }
<<<<<<< HEAD
    void startLoop(const char* exe_name)
||||||| merged common ancestors
    void startLoop()
=======
    Context& context() override { return m_context; }
    void startLoop()
>>>>>>> Multiprocess bitcoin
    {
        if (m_loop) return;
        std::promise<void> promise;
        m_loop_thread = std::thread([&] {
            util::ThreadRename("capnp-loop");
<<<<<<< HEAD
            m_loop.emplace(exe_name, &IpcLogFn, nullptr);
||||||| merged common ancestors
            m_loop.emplace(m_exe_name, &IpcLogFn, nullptr);
=======
            m_loop.emplace(m_exe_name, &IpcLogFn, &m_context);
>>>>>>> Multiprocess bitcoin
            {
                std::unique_lock<std::mutex> lock(m_loop->m_mutex);
                m_loop->addClient(lock);
            }
            promise.set_value();
            m_loop->loop();
            m_loop.reset();
        });
        promise.get_future().wait();
    }
<<<<<<< HEAD
||||||| merged common ancestors
    const char* m_exe_name;
    interfaces::Init& m_init;
=======
    const char* m_exe_name;
    Context m_context;
>>>>>>> Multiprocess bitcoin
    std::thread m_loop_thread;
    std::optional<mp::EventLoop> m_loop;
};
} // namespace

std::unique_ptr<Protocol> MakeCapnpProtocol() { return MakeUnique<CapnpProtocol>(); }
} // namespace capnp
} // namespace ipc
