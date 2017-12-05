// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <interfaces/ipc.h>

#include <fs.h>
#include <interfaces/init.h>
#include <util/memory.h>
#include <util/strencodings.h>

#include <iostream>
#include <mp/util.h>
#include <tinyformat.h>

namespace interfaces {
namespace {
class IpcProcessImpl : public IpcProcess
{
public:
    IpcProcessImpl(int argc, char* argv[], const char* exe_name, IpcProtocol& protocol)
        : m_argc(argc), m_argv(argv), m_exe_name(exe_name), m_protocol(protocol)
    {
    }
    int spawn(const std::string& new_exe_name, int& pid) override
    {
        return mp::SpawnProcess(pid, [&](int fd) {
            fs::path path = m_argc > 0 ? m_argv[0] : "";
            path.remove_filename();
            path.append(new_exe_name);
            return std::vector<std::string>{path.string(), "-ipcfd", strprintf("%i", fd)};
        });
    }
    int wait(int pid) override { return mp::WaitProcess(pid); }
    bool serve(int& exit_status) override
    {
        if (m_argc != 3 || strcmp(m_argv[1], "-ipcfd") != 0) {
            return false;
        }
        try {
            int32_t fd = -1;
            if (!ParseInt32(m_argv[2], &fd)) {
                throw std::runtime_error(strprintf("Invalid -ipcfd number '%s'", m_argv[2]));
            }
            m_protocol.serve(fd);
            exit_status = EXIT_SUCCESS;
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
            exit_status = EXIT_FAILURE;
        }
        return true;
    }
    int m_argc;
    char** m_argv;
    const char* m_exe_name;
    IpcProtocol& m_protocol;
};
} // namespace

std::unique_ptr<IpcProcess> MakeIpcProcess(int argc, char* argv[], const char* exe_name, IpcProtocol& protocol)
{
    return MakeUnique<IpcProcessImpl>(argc, argv, exe_name, protocol);
}
} // namespace interfaces
