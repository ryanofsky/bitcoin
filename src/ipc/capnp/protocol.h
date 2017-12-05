// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_IPC_CAPNP_PROTOCOL_H
#define BITCOIN_IPC_CAPNP_PROTOCOL_H

#include <memory>

namespace interfaces {
class Init;
} // namespace interfaces

namespace ipc {
class Protocol;
namespace capnp {
std::unique_ptr<Protocol> MakeCapnpProtocol(const char* exe_name, interfaces::Init& init);
} // namespace capnp
} // namespace ipc

#endif // BITCOIN_IPC_CAPNP_PROTOCOL_H
