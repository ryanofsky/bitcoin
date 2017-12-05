// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_INTERFACES_INIT_H
#define BITCOIN_INTERFACES_INIT_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

struct NodeContext;

namespace interfaces {
class Base;
class Chain;
class ChainClient;
class IpcProcess;
class IpcProtocol;
class Node;
struct NodeClientParam;
struct NodeServerParam;

//! Interface allowing multiprocess code to create other interfaces on startup.
class Init
{
public:
    virtual ~Init() = default;
    virtual std::unique_ptr<Node> makeNode() = 0;
    virtual std::unique_ptr<Chain> makeChain() = 0;
    virtual std::unique_ptr<ChainClient> makeWalletClient(Chain& chain, std::vector<std::string> wallet_filenames) = 0;
};

//! Specialization of Init for current process.
class LocalInit : public Init
{
public:
    LocalInit(const char* exe_name, const char* log_suffix);
    ~LocalInit() override;
    std::unique_ptr<Node> makeNode() override;
    std::unique_ptr<Chain> makeChain() override;
    std::unique_ptr<ChainClient> makeWalletClient(Chain& chain, std::vector<std::string> wallet_filenames) override;
    using MakeClientFn = std::function<Base&(Init&)>;
    void spawnProcess(const std::string& new_exe_name, const MakeClientFn& make_client);
    //! Do extra initialization needed to initialize the second gui/node/wallet
    //! process when code is running in a new process, instead of the process
    //! that called it.
    //!
    //! Because gui/node/wallet processes all have slightly different init
    //! sequences (this should be cleaned up), the implementation of this method
    //! varies between the three processes, but in all cases brings them up to
    //! the point where global ECC crypto, config, and chain variables are
    //! initialized and logging is started.
    //!
    //! - For bitcoind and bitcoin-qt, this is unimplemented and isn't called.
    //!
    //! - For bitcoin-gui, this is called after the normal ECC and config parsing
    //!   code before spawning/connecting and just starts logging.
    //!
    //! - For bitcoin-node, this is not called by default, but if bitcoin-node
    //!   is spawned internally by the gui, this is called from
    //!   Node.baseInitialize after receiving GUI GlobalArgs catch up the
    //!   spawned process to current point in GUI initialization sequence
    //!   instead of its own.
    //!
    //! - For bitcoin-wallet, this is not called by default, but if
    //!   bitcoin-wallet is spawned internally by the node, it's called from
    //!   Init.makeWalletClient after receiving node GlobalArgs to prepare
    //!   spawned process for the node's initialization sequence of loading
    //!   wallet files and registering RPCs.
    virtual void initProcess() {}
    virtual void makeNodeServer(NodeServerParam&) {}
    virtual void makeNodeClient(NodeClientParam&) {}
    virtual NodeContext& node();
    const char* m_exe_name;
    const char* m_log_suffix;
    std::unique_ptr<IpcProtocol> m_protocol;
    std::unique_ptr<IpcProcess> m_process;
};

//! Create interface pointers used by current process.
std::unique_ptr<LocalInit> MakeInit(int argc, char* argv[]);
} // namespace interfaces

#endif // BITCOIN_INTERFACES_INIT_H
