# Copyright (c) 2024 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0xc77d03df6a41b505;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("ipc/capnp/mining.h");
$Proxy.includeTypes("ipc/capnp/mining-types.h");

using Common = import "common.capnp";

interface Mining $Proxy.wrap("interfaces::Mining") {
    startNode @0 (context :Proxy.Context, settings: Common.Settings, argv: List(Text) $Proxy.count(2)) -> (result: Bool);
    stopNode @1 (context :Proxy.Context) -> (exitStatus: Int32, result: Bool);
    isTestChain @2 (context :Proxy.Context) -> (result: Bool);
    isInitialBlockDownload @3 (context :Proxy.Context) -> (result: Bool);
    getTipHash @4 (context :Proxy.Context) -> (result: Data);
    createNewBlock @5 (context :Proxy.Context, scriptPubKey: Data, useMempool: Bool) -> (result: BlockTemplate);
    processNewBlock @6 (context :Proxy.Context, block: Data) -> (newBlock: Bool, result: Bool);
    getTransactionsUpdated @7 (context :Proxy.Context) -> (result: UInt32);
    testBlockValidity @8 (context :Proxy.Context, block: Data, checkMerkleRoot: Bool) -> (state: BlockValidationState, result: Bool);
}

struct BlockTemplate $Proxy.wrap("node::CBlockTemplate")
{
    block @0 :Data;
    vTxFees @1 :List(UInt64);
    vTxSigOpsCost @2 :List(UInt64);
    vchCoinbaseCommitment @3 :Data;
}

struct BlockValidationState {
}
