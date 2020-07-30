// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_CAPNP_IPC_H
#define BITCOIN_INTERFACES_CAPNP_IPC_H

#include <interfaces/ipc.h>
#include <logging.h>

#include <memory>
#include <string>
#include <tinyformat.h>

#define LogIpc(loop, format, ...) \
    LogPrint(::BCLog::IPC, "{%s} " format, mp::LongThreadName((loop).m_exe_name), ##__VA_ARGS__)

namespace interfaces {
namespace capnp {
std::unique_ptr<IpcProtocol> MakeCapnpProtocol(const char* exe_name, LocalInit& init);
} // namespace capnp
} // namespace interfaces

#endif // BITCOIN_INTERFACES_CAPNP_IPC_H
