# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xf2c5cfa319406aa6;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("interfaces/chain.h");
$Proxy.include("interfaces/echo.h");
$Proxy.include("interfaces/init.h");
$Proxy.include("interfaces/mining.h");
$Proxy.include("interfaces/node.h");
$Proxy.includeTypes("ipc/capnp/init-types.h");

<<<<<<< HEAD
using Chain = import "chain.capnp";
||||||| parent of 9ca113f326fc (multiprocess: Make bitcoin-node spawn a bitcoin-wallet process)
=======
using Chain = import "chain.capnp";
using Common = import "common.capnp";
>>>>>>> 9ca113f326fc (multiprocess: Make bitcoin-node spawn a bitcoin-wallet process)
using Echo = import "echo.capnp";
using Mining = import "mining.capnp";
using Node = import "node.capnp";
using Wallet = import "wallet.capnp";

interface Init $Proxy.wrap("interfaces::Init") {
    construct @0 (threadMap: Proxy.ThreadMap) -> (threadMap :Proxy.ThreadMap);
    makeEcho @1 (context :Proxy.Context) -> (result :Echo.Echo);
    makeMining @2 (context :Proxy.Context) -> (result :Mining.Mining);
<<<<<<< HEAD
    makeChain @3 (context :Proxy.Context) -> (result :Chain.Chain);
||||||| parent of fd311f2af13d (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
=======
    makeNode @3 (context :Proxy.Context) -> (result :Node.Node);
<<<<<<< HEAD
>>>>>>> fd311f2af13d (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
||||||| parent of 9ca113f326fc (multiprocess: Make bitcoin-node spawn a bitcoin-wallet process)
=======
    makeChain @4 (context :Proxy.Context) -> (result :Chain.Chain);
    makeWalletLoader @5 (context :Proxy.Context, globalArgs :Common.GlobalArgs, chain :Chain.Chain) -> (result :Wallet.WalletLoader);
>>>>>>> 9ca113f326fc (multiprocess: Make bitcoin-node spawn a bitcoin-wallet process)
}
