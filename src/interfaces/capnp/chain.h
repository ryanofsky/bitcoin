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

//! Specialization of ChainNotifications client to deal with different
//! capitalization of method names. Cap'n Proto requires all method names to be
//! lowercase, so this forwards the calls.
template <>
class mp::ProxyClientCustom<interfaces::capnp::messages::ChainNotifications, interfaces::Chain::Notifications>
    : public mp::ProxyClientBase<interfaces::capnp::messages::ChainNotifications, interfaces::Chain::Notifications>
{
public:
    using ProxyClientBase::ProxyClientBase;
    void TransactionAddedToMempool(const CTransactionRef& tx) override;
    void TransactionRemovedFromMempool(const CTransactionRef& ptx) override;
    void BlockConnected(const CBlock& block, const std::vector<CTransactionRef>& tx_conflicted, int height) override;
    void BlockDisconnected(const CBlock& block, int height) override;
    void UpdatedBlockTip() override;
    void ChainStateFlushed(const CBlockLocator& locator) override;
};

#endif // BITCOIN_INTERFACES_CAPNP_CHAIN_H
