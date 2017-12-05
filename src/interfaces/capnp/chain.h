// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_CAPNP_CHAIN_H
#define BITCOIN_INTERFACES_CAPNP_CHAIN_H

#include <interfaces/capnp/chain.capnp.h>
#include <interfaces/chain.h>
#include <mp/proxy.h>
#include <rpc/server.h>

//! Specialization of ChainClient proxy server needed hold a CSCheduler instance.
template <>
struct mp::ProxyServerCustom<interfaces::capnp::messages::ChainClient, interfaces::ChainClient>
    : public mp::ProxyServerBase<interfaces::capnp::messages::ChainClient, interfaces::ChainClient>
{
public:
    ProxyServerCustom(interfaces::ChainClient* impl, bool owned, mp::Connection& connection);
    void invokeDestroy();

    std::unique_ptr<CScheduler> m_scheduler;
    std::future<void> m_result;
};

#endif // BITCOIN_INTERFACES_CAPNP_CHAIN_H
