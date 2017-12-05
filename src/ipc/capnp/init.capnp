# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xf2c5cfa319406aa6;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("interfaces/echo.h");
$Proxy.include("interfaces/init.h");
<<<<<<< HEAD
$Proxy.include("interfaces/mining.h");
||||||| parent of 3a10e9a55cff (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
=======
$Proxy.include("interfaces/node.h");
>>>>>>> 3a10e9a55cff (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
$Proxy.includeTypes("ipc/capnp/init-types.h");

using Echo = import "echo.capnp";
<<<<<<< HEAD
using Mining = import "mining.capnp";
||||||| parent of 3a10e9a55cff (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
=======
using Node = import "node.capnp";
>>>>>>> 3a10e9a55cff (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)

interface Init $Proxy.wrap("interfaces::Init") {
    construct @0 (threadMap: Proxy.ThreadMap) -> (threadMap :Proxy.ThreadMap);
    makeEcho @1 (context :Proxy.Context) -> (result :Echo.Echo);
<<<<<<< HEAD
    makeMining @2 (context :Proxy.Context) -> (result :Mining.Mining);
||||||| parent of 3a10e9a55cff (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
=======
    makeNode @2 (context :Proxy.Context) -> (result :Node.Node);
>>>>>>> 3a10e9a55cff (multiprocess: Make bitcoin-gui spawn a bitcoin-node process)
}
