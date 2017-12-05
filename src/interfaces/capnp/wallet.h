// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_CAPNP_WALLET_H
#define BITCOIN_INTERFACES_CAPNP_WALLET_H

#include <interfaces/capnp/chain.capnp.proxy.h>
#include <interfaces/capnp/wallet.capnp.h>
#include <interfaces/wallet.h>
#include <mp/proxy.h>

//! Specialization of WalletClient proxy server needed hold a CSCheduler instance.
template <>
struct mp::ProxyServerCustom<interfaces::capnp::messages::WalletClient, interfaces::WalletClient>
    : public mp::ProxyServerBase<interfaces::capnp::messages::WalletClient, interfaces::WalletClient>
{
 public:
    using ProxyServerBase::ProxyServerBase;
    void invokeDestroy();

    std::unique_ptr<CScheduler> m_scheduler;
    std::future<void> m_result;
};

#endif // BITCOIN_INTERFACES_CAPNP_WALLET_H
