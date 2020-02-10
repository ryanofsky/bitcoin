# Copyright (c) 2019 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xf2c5cfa319406aa6;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("interfaces::capnp::messages");

using Chain = import "chain.capnp";
using Common = import "common.capnp";
using Node = import "node.capnp";
using Wallet = import "wallet.capnp";
using Proxy = import "/mp/proxy.capnp";

interface Init $Proxy.wrap("interfaces::Init") {
    construct @0 (threadMap: Proxy.ThreadMap) -> (threadMap :Proxy.ThreadMap);
    makeNode @1 (context :Proxy.Context) -> (result :Node.Node);
    makeChain @2 (context :Proxy.Context) -> (result :Chain.Chain);
    makeWalletClient @3 (context :Proxy.Context, globalArgs :Common.GlobalArgs, chain :Chain.Chain, walletFilenames :List(Text)) -> (result :Chain.ChainClient);
}
