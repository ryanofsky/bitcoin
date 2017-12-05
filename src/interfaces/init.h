// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_INIT_H
#define BITCOIN_INTERFACES_INIT_H

#include <util/ref.h>

#include <fs.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct NodeContext;

namespace interfaces {
class Base;
class IpcProcess;
class IpcProtocol;

//! Interface allowing multiprocess code to create other interfaces on startup.
class Init
{
public:
    virtual ~Init() = default;
};

//! Specialization of Init for current process.
class LocalInit : public Init
{
public:
    LocalInit(const char* exe_name, const char* log_suffix);
    ~LocalInit() override;
    using MakeClientFn = std::function<Base&(Init&)>;
    void spawnProcess(const std::string& new_exe_name, const MakeClientFn& make_client);
    virtual NodeContext& node();
    const char* m_exe_name;
    const char* m_log_suffix;
    std::unique_ptr<IpcProtocol> m_protocol;
    std::unique_ptr<IpcProcess> m_process;
    util::Ref m_request_context;
};

//! Create interface pointers used by current process.
std::unique_ptr<LocalInit> MakeInit(int argc, char* argv[]);
} // namespace interfaces

#endif // BITCOIN_INTERFACES_INIT_H
