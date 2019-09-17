// Copyright (c) 2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_NODE_NODE_H
#define BITCOIN_NODE_NODE_H

#include <memory>
#include <vector>

class BanMan;
class CConnman;
class PeerLogicValidation;
namespace interfaces {
class Chain;
class ChainClient;
} // namespace interfaces

//! Node struct containing references to chain state and connection state.
//!
//! This is used by init, rpc, and test code to pass object references around
//! without needing to declare the same variables and parameters repeatedly, or
//! to use globals. More variables could be added to this struct (particularly
//! references to validation and mempool objects) to eliminate use of globals
//! and make code more modular and testable. The struct isn't intended to have
//! any member functions. It should just be a collection of references that can
//! be used without pulling in unwanted dependencies or functionality.
struct Node
{
    std::unique_ptr<CConnman> connman;
    std::unique_ptr<PeerLogicValidation> peer_logic;
    std::unique_ptr<BanMan> banman;
    std::unique_ptr<interfaces::Chain> chain;
    std::vector<std::unique_ptr<interfaces::ChainClient>> chain_clients;

    //! Declare default constructor and destructor that are not inline, so code
    //! instantiating the Node struct doesn't need to #include class
    //! definitions for all the unique_ptr members.
    Node();
    ~Node();
};

#endif // BITCOIN_NODE_NODE_H
