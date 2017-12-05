// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/init.h>
#include <ipc/capnp/init.capnp.h>
#include <ipc/capnp/init.capnp.proxy.h>
#include <ipc/capnp/protocol.h>
#include <ipc/exception.h>
#include <ipc/protocol.h>
#include <kj/async.h>
#include <logging.h>
#include <mp/proxy-io.h>
#include <mp/util.h>
#include <optional.h>
#include <util/memory.h>
#include <util/threadnames.h>

#include <assert.h>
#include <errno.h>
#include <future>
#include <memory>
#include <mutex>
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
    CapnpProtocol(const char* exe_name, interfaces::Init& init) : m_exe_name(exe_name), m_init(init) {}
    ~CapnpProtocol() noexcept(true)
    {
        if (m_loop) {
            std::unique_lock<std::mutex> lock(m_loop->m_mutex);
            m_loop->removeClient(lock);
        }
        if (m_loop_thread.joinable()) m_loop_thread.join();
        assert(!m_loop);
    };
    std::unique_ptr<interfaces::Init> connect(int fd) override
    {
        startLoop();
        return mp::ConnectStream<messages::Init>(*m_loop, fd);
    }
    void serve(int fd) override
    {
        assert(!m_loop);
        mp::g_thread_context.thread_name = mp::ThreadName(m_exe_name);
        m_loop.emplace(m_exe_name, &IpcLogFn, nullptr);
        mp::ServeStream<messages::Init>(*m_loop, fd, m_init);
        m_loop->loop();
        m_loop.reset();
    }
    void startLoop()
    {
        if (m_loop) return;
        std::promise<void> promise;
        m_loop_thread = std::thread([&] {
            util::ThreadRename("capnp-loop");
            m_loop.emplace(m_exe_name, &IpcLogFn, nullptr);
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
    const char* m_exe_name;
    interfaces::Init& m_init;
    std::thread m_loop_thread;
    boost::optional<mp::EventLoop> m_loop;
};
} // namespace

std::unique_ptr<Protocol> MakeCapnpProtocol(const char* exe_name, interfaces::Init& init)
{
    return MakeUnique<CapnpProtocol>(exe_name, init);
}
} // namespace capnp
} // namespace ipc
