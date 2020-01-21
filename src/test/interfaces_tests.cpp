// Copyright (c) 2020 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chainparams.h>
#include <consensus/validation.h>
#include <interfaces/chain.h>
#include <script/standard.h>
#include <test/util/setup_common.h>
#include <validation.h>

#include <boost/test/unit_test.hpp>

BOOST_FIXTURE_TEST_SUITE(interfaces_tests, TestChain100Setup)

BOOST_AUTO_TEST_CASE(findBlock)
{
    auto chain = interfaces::MakeChain(m_node);
    auto& active = ::ChainActive();
    int64_t time_mtp = -1;
    BOOST_CHECK(chain->findBlock(active[20]->GetBlockHash(), nullptr, nullptr, nullptr, &time_mtp));
    BOOST_CHECK_EQUAL(time_mtp, active[20]->GetMedianTimePast());
}

BOOST_AUTO_TEST_CASE(findAncestorByHeight)
{
    auto chain = interfaces::MakeChain(m_node);
    auto& active = ::ChainActive();
    BOOST_CHECK_EQUAL(chain->findAncestorByHeight(active[20]->GetBlockHash(), 10), active[10]->GetBlockHash());
    BOOST_CHECK_EQUAL(chain->findAncestorByHeight(active[10]->GetBlockHash(), 20), uint256());
}

BOOST_AUTO_TEST_CASE(findAncestorByHash)
{
    auto chain = interfaces::MakeChain(m_node);
    auto& active = ::ChainActive();
    int height = -1;
    BOOST_CHECK(chain->findAncestorByHash(active[20]->GetBlockHash(), active[10]->GetBlockHash(), &height));
    BOOST_CHECK_EQUAL(height, 10);
    BOOST_CHECK(!chain->findAncestorByHash(active[10]->GetBlockHash(), active[20]->GetBlockHash()));
}

BOOST_AUTO_TEST_CASE(findFork)
{
    auto chain = interfaces::MakeChain(m_node);
    auto& active = ::ChainActive();
    auto* orig_tip = active.Tip();
    for (int i = 0; i < 10; ++i) {
        BlockValidationState state;
        ChainstateActive().InvalidateBlock(state, Params(), ChainActive().Tip());
    }
    BOOST_CHECK_EQUAL(active.Height(), orig_tip->nHeight - 10);
    coinbaseKey.MakeNewKey(true);
    for (int i = 0; i < 20; ++i) {
        CreateAndProcessBlock({}, GetScriptForRawPubKey(coinbaseKey.GetPubKey()));
    }
    BOOST_CHECK_EQUAL(active.Height(), orig_tip->nHeight + 10);
    uint256 fork_hash;
    int fork_height;
    BOOST_CHECK_EQUAL(*chain->findFork(orig_tip->GetBlockHash(), active.Tip()->GetBlockHash(), &fork_hash, &fork_height), orig_tip->nHeight);
    BOOST_CHECK_EQUAL(fork_height, orig_tip->nHeight - 10);
    BOOST_CHECK_EQUAL(fork_hash, active[fork_height]->GetBlockHash());
}

BOOST_AUTO_TEST_SUITE_END()
