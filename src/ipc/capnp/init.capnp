# Copyright (c) 2021-present The Bitcoin Core developers
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

using Chain = import "chain.capnp";
using Common = import "common.capnp";
using Echo = import "echo.capnp";
using Mining = import "mining.capnp";
<<<<<<< HEAD
using Rpc = import "rpc.capnp";
||||||| parent of 3bcfd4eac03 (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
=======
using Node = import "node.capnp";
<<<<<<< HEAD
>>>>>>> 3bcfd4eac03 (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
||||||| parent of 00059c7726d (multiprocess: Make bitcoin-node spawn a bitcoin-wallet process)
=======
using Wallet = import "wallet.capnp";
>>>>>>> 00059c7726d (multiprocess: Make bitcoin-node spawn a bitcoin-wallet process)

interface Init $Proxy.wrap("interfaces::Init") {
    construct @0 (threadMap: Proxy.ThreadMap) -> (threadMap :Proxy.ThreadMap);
    makeEcho @1 (context :Proxy.Context) -> (result :Echo.Echo);
    makeMining @3 (context :Proxy.Context) -> (result :Mining.Mining);
<<<<<<< HEAD
    makeRpc @4 (context :Proxy.Context) -> (result :Rpc.Rpc);
    makeChain @5 (context :Proxy.Context) -> (result :Chain.Chain);
||||||| parent of 3bcfd4eac03 (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
    makeChain @4 (context :Proxy.Context) -> (result :Chain.Chain);
=======
    makeChain @4 (context :Proxy.Context) -> (result :Chain.Chain);
    makeNode @5 (context :Proxy.Context) -> (result :Node.Node);
<<<<<<< HEAD
>>>>>>> 3bcfd4eac03 (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
||||||| parent of 00059c7726d (multiprocess: Make bitcoin-node spawn a bitcoin-wallet process)
=======
    makeWalletLoader @6 (context :Proxy.Context, globalArgs :Common.GlobalArgs, chain :Chain.Chain) -> (result :Wallet.WalletLoader);
>>>>>>> 00059c7726d (multiprocess: Make bitcoin-node spawn a bitcoin-wallet process)

    # DEPRECATED: no longer supported; server returns an error.
    makeMiningOld2 @2 () -> ();
}
