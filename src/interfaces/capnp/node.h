// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_CAPNP_NODE_H
#define BITCOIN_INTERFACES_CAPNP_NODE_H

#include <interfaces/capnp/node.capnp.h>
#include <interfaces/node.h>
#include <mp/proxy.h>
#include <rpc/server.h>
#include <scheduler.h>

#include <memory>
#include <string>

class RPCTimerInterface;

//! Specialization of Node client to deal with argument & config handling across
//! processes. If node and node client are running in same process it's
//! sufficient to only call parseParameters, softSetArgs, etc methods only on the
//! node object, but if node is running in a different process, the calls need to
//! be made repeated locally as well to update the state of the client process..
template <>
class mp::ProxyClientCustom<interfaces::capnp::messages::Node, interfaces::Node>
    : public mp::ProxyClientBase<interfaces::capnp::messages::Node, interfaces::Node>
{
public:
    using ProxyClientBase::ProxyClientBase;
    void setupServerArgs() override;
    bool parseParameters(int argc, const char* const argv[], std::string& error) override;
    void forceSetArg(const std::string& arg, const std::string& value) override;
    bool softSetArg(const std::string& arg, const std::string& value) override;
    bool softSetBoolArg(const std::string& arg, bool value) override;
    bool readConfigFiles(std::string& error) override;
    void selectParams(const std::string& network) override;
    bool initSettings(std::string& error) override;
    bool baseInitialize() override;
};

//! Specialization of Node proxy server needed to add m_timer_interface
//! member used by rpcSetTimerInterfaceIfUnset and rpcUnsetTimerInterface
//! methods.
template <>
struct mp::ProxyServerCustom<interfaces::capnp::messages::Node, interfaces::Node>
    : public mp::ProxyServerBase<interfaces::capnp::messages::Node, interfaces::Node>
{
public:
    using ProxyServerBase::ProxyServerBase;
    std::unique_ptr<::RPCTimerInterface> m_timer_interface;
};

#endif // BITCOIN_INTERFACES_CAPNP_NODE_H
