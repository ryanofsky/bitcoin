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
using Node = import "node.capnp";
using Rpc = import "rpc.capnp";
using Wallet = import "wallet.capnp";

interface Init $Proxy.wrap("interfaces::Init") {
    construct @0 (threadMap: Proxy.ThreadMap) -> (threadMap :Proxy.ThreadMap);
    makeEcho @1 (context :Proxy.Context) -> (result :Echo.Echo);
    makeMining @3 (context :Proxy.Context) -> (result :Mining.Mining);
    makeRpc @4 (context :Proxy.Context) -> (result :Rpc.Rpc);
    makeChain @5 (context :Proxy.Context) -> (result :Chain.Chain);
<<<<<<< HEAD
    # Unstable chain interface for Bitcoin Core's own clients. This method is
    # renumbered whenever UnstableChain changes incompatibly, so old clients
    # get an error instead of calling the wrong methods.
    makeUnstableChain @6 (context :Proxy.Context) -> (result :Chain.UnstableChain) $Proxy.name("makeChain");
||||||| parent of 6cd132fc0bc (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
=======
    makeNode @6 (context :Proxy.Context) -> (result :Node.Node);
<<<<<<< HEAD
>>>>>>> 6cd132fc0bc (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
||||||| parent of d33fadef8ed (multiprocess: Make bitcoin-node spawn a bitcoin-wallet process)
=======
    makeWalletLoader @7 (context :Proxy.Context, globalArgs :Common.GlobalArgs, chain :Chain.Chain) -> (result :Wallet.WalletLoader);
>>>>>>> d33fadef8ed (multiprocess: Make bitcoin-node spawn a bitcoin-wallet process)

    # DEPRECATED: no longer supported; server returns an error.
    makeMiningOld2 @2 () -> ();
}
