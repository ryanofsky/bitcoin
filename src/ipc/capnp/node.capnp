# Copyright (c) 2021 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.

@0x92546c47dc734b2e;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("ipc::capnp::messages");

using Proxy = import "/mp/proxy.capnp";
$Proxy.include("ipc/capnp/node.h");
$Proxy.includeTypes("ipc/capnp/node-types.h");

using Common = import "common.capnp";
using Handler = import "handler.capnp";
using Wallet = import "wallet.capnp";

interface Node $Proxy.wrap("interfaces::Node") {
    destroy @0 (context :Proxy.Context) -> ();
    initLogging @1 (context :Proxy.Context) -> ();
    initParameterInteraction @2 (context :Proxy.Context) -> ();
    getWarnings @3 (context :Proxy.Context) -> (result :Common.BilingualStr);
    getLogCategories @4 (context :Proxy.Context) -> (result :UInt32);
    baseInitialize @5 (context :Proxy.Context, globalArgs :Common.GlobalArgs) -> (error :Text $Proxy.exception("std::exception"), result :Bool);
    appInitMain @6 (context :Proxy.Context) -> (tipInfo :BlockAndHeaderTipInfo, error :Text $Proxy.exception("std::exception"), result :Bool);
    appShutdown @7 (context :Proxy.Context) -> ();
    startShutdown @8 (context :Proxy.Context) -> ();
    shutdownRequested @9 (context :Proxy.Context) -> (result :Bool);
    isSettingIgnored @10 (name :Text) -> (result: Bool);
    getPersistentSetting @11 (name :Text) -> (result: Text);
    updateRwSetting @12 (name :Text, value :Text) -> ();
    forceSetting @13 (name :Text, value :Text) -> ();
    resetSettings @14 () -> ();
    mapPort @15 (context :Proxy.Context, useUPnP :Bool, useNatPnP :Bool) -> ();
    getProxy @16 (context :Proxy.Context, net :Int32) -> (proxyInfo :ProxyInfo, result :Bool);
    getNodeCount @17 (context :Proxy.Context, flags :Int32) -> (result :UInt64);
    getNodesStats @18 (context :Proxy.Context) -> (stats :List(NodeStats), result :Bool);
    getBanned @19 (context :Proxy.Context) -> (banmap :Banmap, result :Bool);
    ban @20 (context :Proxy.Context, netAddr :Data, banTimeOffset :Int64) -> (result :Bool);
    unban @21 (context :Proxy.Context, ip :Data) -> (result :Bool);
    disconnectByAddress @22 (context :Proxy.Context, address :Data) -> (result :Bool);
    disconnectById @23 (context :Proxy.Context, id :Int64) -> (result :Bool);
    listExternalSigners @24 (context :Proxy.Context) -> (result :List(ExternalSigner));
    getTotalBytesRecv @25 (context :Proxy.Context) -> (result :Int64);
    getTotalBytesSent @26 (context :Proxy.Context) -> (result :Int64);
    getMempoolSize @27 (context :Proxy.Context) -> (result :UInt64);
    getMempoolDynamicUsage @28 (context :Proxy.Context) -> (result :UInt64);
    getHeaderTip @29 (context :Proxy.Context) -> (height :Int32, blockTime :Int64, result :Bool);
    getNumBlocks @30 (context :Proxy.Context) -> (result :Int32);
    getBestBlockHash @31 (context :Proxy.Context) -> (result :Data);
    getLastBlockTime @32 (context :Proxy.Context) -> (result :Int64);
    getVerificationProgress @33 (context :Proxy.Context) -> (result :Float64);
    isInitialBlockDownload @34 (context :Proxy.Context) -> (result :Bool);
    isLoadingBlocks @35 (context :Proxy.Context) -> (result :Bool);
    setNetworkActive @36 (context :Proxy.Context, active :Bool) -> ();
    getNetworkActive @37 (context :Proxy.Context) -> (result :Bool);
    getDustRelayFee @38 (context :Proxy.Context) -> (result :Data);
    executeRpc @39 (context :Proxy.Context, command :Text, params :Text, uri :Text) -> (error :Text $Proxy.exception("std::exception"), rpcError :Text $Proxy.exception("UniValue"), result :Text);
    listRpcCommands @40 (context :Proxy.Context) -> (result :List(Text));
    rpcSetTimerInterfaceIfUnset @41 (context :Proxy.Context, iface :Void) -> ();
    rpcUnsetTimerInterface @42 (context :Proxy.Context, iface :Void) -> ();
    getUnspentOutput @43 (context :Proxy.Context, output :Data) -> (result :Data);
    broadcastTransaction @44 (context :Proxy.Context, tx: Data, maxTxFee :Int64) -> (error: Text, result :Int32);
    customWalletLoader @45 (context :Proxy.Context) -> (result :Wallet.WalletLoader) $Proxy.name("walletLoader");
    handleInitMessage @46 (context :Proxy.Context, callback :InitMessageCallback) -> (result :Handler.Handler);
    handleMessageBox @47 (context :Proxy.Context, callback :MessageBoxCallback) -> (result :Handler.Handler);
    handleQuestion @48 (context :Proxy.Context, callback :QuestionCallback) -> (result :Handler.Handler);
    handleShowProgress @49 (context :Proxy.Context, callback :ShowNodeProgressCallback) -> (result :Handler.Handler);
    handleInitWallet @50 (context :Proxy.Context, callback :InitWalletCallback) -> (result :Handler.Handler);
    handleNotifyNumConnectionsChanged @51 (context :Proxy.Context, callback :NotifyNumConnectionsChangedCallback) -> (result :Handler.Handler);
    handleNotifyNetworkActiveChanged @52 (context :Proxy.Context, callback :NotifyNetworkActiveChangedCallback) -> (result :Handler.Handler);
    handleNotifyAlertChanged @53 (context :Proxy.Context, callback :NotifyAlertChangedCallback) -> (result :Handler.Handler);
    handleBannedListChanged @54 (context :Proxy.Context, callback :BannedListChangedCallback) -> (result :Handler.Handler);
    handleNotifyBlockTip @55 (context :Proxy.Context, callback :NotifyBlockTipCallback) -> (result :Handler.Handler);
    handleNotifyHeaderTip @56 (context :Proxy.Context, callback :NotifyHeaderTipCallback) -> (result :Handler.Handler);
}

interface ExternalSigner $Proxy.wrap("interfaces::ExternalSigner") {
    destroy @0 (context :Proxy.Context) -> ();
    getName @1 (context :Proxy.Context) -> (result :Text);
}

interface InitMessageCallback $Proxy.wrap("ProxyCallback<interfaces::Node::InitMessageFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, message :Text) -> ();
}

interface MessageBoxCallback $Proxy.wrap("ProxyCallback<interfaces::Node::MessageBoxFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, message :Common.BilingualStr, caption :Text, style :UInt32) -> (result :Bool);
}

interface QuestionCallback $Proxy.wrap("ProxyCallback<interfaces::Node::QuestionFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, message :Common.BilingualStr, nonInteractiveMessage :Text, caption :Text, style :UInt32) -> (result :Bool);
}

interface ShowNodeProgressCallback $Proxy.wrap("ProxyCallback<interfaces::Node::ShowProgressFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, title :Text, progress :Int32, resumePossible :Bool) -> ();
}

interface InitWalletCallback $Proxy.wrap("ProxyCallback<interfaces::Node::InitWalletFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context) -> ();
}

interface NotifyNumConnectionsChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Node::NotifyNumConnectionsChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, newNumConnections :Int32) -> ();
}

interface NotifyNetworkActiveChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Node::NotifyNetworkActiveChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, networkActive :Bool) -> ();
}

interface NotifyAlertChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Node::NotifyAlertChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context) -> ();
}

interface BannedListChangedCallback $Proxy.wrap("ProxyCallback<interfaces::Node::BannedListChangedFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context) -> ();
}

interface NotifyBlockTipCallback $Proxy.wrap("ProxyCallback<interfaces::Node::NotifyBlockTipFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, syncState: Int32, tip: BlockTip, verificationProgress :Float64) -> ();
}

interface NotifyHeaderTipCallback $Proxy.wrap("ProxyCallback<interfaces::Node::NotifyHeaderTipFn>") {
    destroy @0 (context :Proxy.Context) -> ();
    call @1 (context :Proxy.Context, syncState: Int32, tip: BlockTip, verificationProgress :Float64) -> ();
}

struct ProxyInfo $Proxy.wrap("::Proxy") {
    proxy @0 :Data;
    randomizeCredentials @1 :Bool $Proxy.name("randomize_credentials");
}

struct NodeStats $Proxy.wrap("CNodeStats") {
    nodeid @0 :Int64 $Proxy.name("nodeid");
    lastSend @1 :Int64 $Proxy.name("m_last_send");
    lastRecv @2 :Int64 $Proxy.name("m_last_recv");
    lastTXTime @3 :Int64 $Proxy.name("m_last_tx_time");
    lastBlockTime @4 :Int64 $Proxy.name("m_last_block_time");
    timeConnected @5 :Int64 $Proxy.name("m_connected");
    timeOffset @6 :Int64 $Proxy.name("nTimeOffset");
    addrName @7 :Text $Proxy.name("m_addr_name");
    version @8 :Int32 $Proxy.name("nVersion");
    cleanSubVer @9 :Text $Proxy.name("cleanSubVer");
    inbound @10 :Bool $Proxy.name("fInbound");
    bip152HighbandwidthTo @11 :Bool $Proxy.name("m_bip152_highbandwidth_to");
    bip152HighbandwidthFrom @12 :Bool $Proxy.name("m_bip152_highbandwidth_from");
    startingHeight @13 :Int32 $Proxy.name("m_starting_height");
    sendBytes @14 :UInt64 $Proxy.name("nSendBytes");
    sendBytesPerMsgType @15 :List(Common.PairStr64) $Proxy.name("mapSendBytesPerMsgType");
    recvBytes @16 :UInt64 $Proxy.name("nRecvBytes");
    recvBytesPerMsgType @17 :List(Common.PairStr64) $Proxy.name("mapRecvBytesPerMsgType");
    permissionFlags @18 :Int32 $Proxy.name("m_permission_flags");
    pingTime @19 :Int64 $Proxy.name("m_last_ping_time");
    minPingTime @20 :Int64 $Proxy.name("m_min_ping_time");
    addrLocal @21 :Text $Proxy.name("addrLocal");
    addr @22 :Data $Proxy.name("addr");
    addrBind @23 :Data $Proxy.name("addrBind");
    network @24 :Int32 $Proxy.name("m_network");
    mappedAs @25 :UInt32 $Proxy.name("m_mapped_as");
    connType @26 :Int32 $Proxy.name("m_conn_type");
    stateStats @27 :NodeStateStats $Proxy.skip;
}

struct NodeStateStats $Proxy.wrap("CNodeStateStats") {
    syncHeight @0 :Int32 $Proxy.name("nSyncHeight");
    commonHeight @1 :Int32 $Proxy.name("nCommonHeight");
    startingHeight @2 :Int32 $Proxy.name("m_starting_height");
    pingWait @3 :Int64 $Proxy.name("m_ping_wait");
    heightInFlight @4 :List(Int32) $Proxy.name("vHeightInFlight");
    addressesProcessed @5 :UInt64 $Proxy.name("m_addr_processed");
    addressesRateLimited @6 :UInt64 $Proxy.name("m_addr_rate_limited");
    addressRelayEnabled @7 :Bool $Proxy.name("m_addr_relay_enabled");
    theirServices @8 :UInt64 $Proxy.name("their_services");
    presyncHeight @9 :Int64 $Proxy.name("presync_height");
}

struct Banmap {
    json @0 :Text;
}

struct BlockTip $Proxy.wrap("interfaces::BlockTip") {
    blockHeight @0 :Int32 $Proxy.name("block_height");
    blockTime @1 :Int64 $Proxy.name("block_time");
    blockHash @2 :Data $Proxy.name("block_hash");
}

struct BlockAndHeaderTipInfo $Proxy.wrap("interfaces::BlockAndHeaderTipInfo") {
    blockHeight @0 :Int32 $Proxy.name("block_height");
    blockTime @1 :Int64 $Proxy.name("block_time");
    headerHeight @2 :Int32 $Proxy.name("header_height");
    headerTime @3 :Int64 $Proxy.name("header_time");
    verificationProgress @4 :Float64 $Proxy.name("verification_progress");
}
