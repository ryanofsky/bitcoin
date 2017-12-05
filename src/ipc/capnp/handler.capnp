# Copyright (c) 2020 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xebd8f46e2f369076;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Proxy = import "/mp/proxy.capnp";

interface Handler $Proxy.wrap("interfaces::Handler") {
    destroy @0 (context :Proxy.Context) -> ();
    disconnect @1 (context :Proxy.Context) -> ();
}
