// Copyright (c) 2025 The BT2C developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>

#include <chain.h>
#include <chainparams.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <validation.h>
#include <kernel.h>
#include <primitives/block.h>
#include <script/script.h>
#include <util/time.h>
#include <key.h>
#include <pow.h>
#include <node/miner.h>

BOOST_FIXTURE_TEST_SUITE(pos_consensus_tests, TestingSetup)

/* Test PoS-only consensus rule enforcement */
BOOST_AUTO_TEST_CASE(pos_only_consensus)
{
    // Create a PoW block (should be rejected)
    CBlock powBlock;
    powBlock.nVersion = 3;
    powBlock.nTime = GetTime();
    
    // Ensure it's recognized as PoW
    BOOST_CHECK(!powBlock.IsProofOfStake());
    BOOST_CHECK(powBlock.IsProofOfWork());
    
    // Test block validation
    const Consensus::Params& consensusParams = Params().GetConsensus();
    BlockValidationState state;
    
    // PoW block should fail validation in BT2C
    bool powResult = CheckBlockHeader(powBlock, state, consensusParams);
    BOOST_CHECK_EQUAL(powResult, false);
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "proof-of-work-rejected");
    
    // Create a PoS block
    CBlock posBlock;
    posBlock.nVersion = 3;
    posBlock.nTime = GetTime();
    posBlock.SetProofOfStake();
    
    // Ensure it's recognized as PoS
    BOOST_CHECK(posBlock.IsProofOfStake());
    BOOST_CHECK(!posBlock.IsProofOfWork());
    
    // PoS block should pass basic validation
    BlockValidationState posState;
    bool posResult = CheckBlockHeader(posBlock, posState, consensusParams);
    BOOST_CHECK_EQUAL(posResult, true);
}

/* Test block time enforcement */
BOOST_AUTO_TEST_CASE(block_time_enforcement)
{
    // Create a block with timestamp too far in the future
    CBlock futureBlock;
    futureBlock.nVersion = 3;
    futureBlock.nTime = GetTime() + 2 * 60 * 60; // 2 hours in the future
    futureBlock.SetProofOfStake();
    
    // Test block validation
    const Consensus::Params& consensusParams = Params().GetConsensus();
    BlockValidationState state;
    
    // Future block should fail validation
    bool result = CheckBlockHeader(futureBlock, state, consensusParams);
    BOOST_CHECK_EQUAL(result, false);
    BOOST_CHECK_EQUAL(state.GetRejectReason(), "time-too-new");
    
    // Create a block with valid timestamp
    CBlock validBlock;
    validBlock.nVersion = 3;
    validBlock.nTime = GetTime();
    validBlock.SetProofOfStake();
    
    // Valid block should pass basic validation
    BlockValidationState validState;
    bool validResult = CheckBlockHeader(validBlock, validState, consensusParams);
    BOOST_CHECK_EQUAL(validResult, true);
}

/* Test block interval enforcement */
BOOST_AUTO_TEST_CASE(block_interval)
{
    // Create a chain of blocks to test block interval
    CBlockIndex* pindexPrev = new CBlockIndex();
    pindexPrev->nHeight = 100;
    pindexPrev->nTime = GetTime() - 60; // 1 minute ago
    
    // Create a block with timestamp too close to previous block
    CBlock tooSoonBlock;
    tooSoonBlock.nVersion = 3;
    tooSoonBlock.nTime = pindexPrev->nTime + 30; // Only 30 seconds after previous block
    tooSoonBlock.SetProofOfStake();
    
    // Test contextual validation
    const Consensus::Params& consensusParams = Params().GetConsensus();
    BlockValidationState state;
    
    // Block too soon should fail validation
    bool result = ContextualCheckBlockHeader(tooSoonBlock, state, pindexPrev, consensusParams);
    BOOST_CHECK_EQUAL(result, false);
    
    // Create a block with valid interval
    CBlock validBlock;
    validBlock.nVersion = 3;
    validBlock.nTime = pindexPrev->nTime + 60; // 60 seconds after previous block
    validBlock.SetProofOfStake();
    
    // Valid block should pass contextual validation
    BlockValidationState validState;
    bool validResult = ContextualCheckBlockHeader(validBlock, validState, pindexPrev, consensusParams);
    BOOST_CHECK_EQUAL(validResult, true);
}

/* Test PoS difficulty adjustment */
BOOST_AUTO_TEST_CASE(pos_difficulty_adjustment)
{
    // Create a chain of blocks
    std::vector<CBlockIndex*> chain;
    int64_t currentTime = GetTime() - 1000 * 60; // Start 1000 minutes ago
    
    // Create 10 blocks with 60-second intervals
    for (int i = 0; i < 10; i++) {
        CBlockIndex* index = new CBlockIndex();
        index->nHeight = i;
        index->nTime = currentTime + i * 60;
        index->nBits = 0x1d00ffff; // Initial difficulty
        
        if (i > 0) {
            index->pprev = chain.back();
        }
        
        chain.push_back(index);
    }
    
    // Calculate next block's difficulty
    const Consensus::Params& consensusParams = Params().GetConsensus();
    unsigned int nextDifficulty = GetNextTargetRequired(chain.back(), consensusParams, true);
    
    // Difficulty should be adjusted based on actual block times
    // This is a simplified test - actual behavior depends on implementation
    BOOST_CHECK(nextDifficulty != 0);
}

/* Test block reward calculation */
BOOST_AUTO_TEST_CASE(block_reward_calculation)
{
    // Test initial block reward (21 BTC)
    CAmount initialReward = GetProofOfStakeReward(0, 0);
    BOOST_CHECK_EQUAL(initialReward, 21 * COIN);
    
    // Test reward after first halving (210,000 blocks)
    CAmount firstHalvingReward = GetProofOfStakeReward(210000, 0);
    BOOST_CHECK_EQUAL(firstHalvingReward, 10.5 * COIN);
    
    // Test reward after second halving (420,000 blocks)
    CAmount secondHalvingReward = GetProofOfStakeReward(420000, 0);
    BOOST_CHECK_EQUAL(secondHalvingReward, 5.25 * COIN);
    
    // Test minimum reward (1 satoshi)
    CAmount minReward = GetProofOfStakeReward(6930000, 0); // Far in the future
    BOOST_CHECK(minReward >= 1); // Should be at least 1 satoshi
}

/* Test PoS block creation */
BOOST_AUTO_TEST_CASE(pos_block_creation)
{
    // Set up wallet for staking
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubKey = key.GetPubKey();
    CScript script = CScript() << ToByteVector(pubKey) << OP_CHECKSIG;
    
    // Create a block template
    std::unique_ptr<CBlockTemplate> blockTemplate = CreateNewBlock(script, true);
    
    // Check that block template was created
    BOOST_CHECK(blockTemplate != nullptr);
    
    // Verify it's a PoS block
    BOOST_CHECK(blockTemplate->block.IsProofOfStake());
    BOOST_CHECK(!blockTemplate->block.IsProofOfWork());
}

/* Test chain selection with PoS blocks */
BOOST_AUTO_TEST_CASE(chain_selection)
{
    // Create two competing chains
    CBlockIndex* commonAncestor = new CBlockIndex();
    commonAncestor->nHeight = 100;
    commonAncestor->nTime = GetTime() - 1000;
    
    // Chain A - higher stake weight
    CBlockIndex* chainA_1 = new CBlockIndex();
    chainA_1->pprev = commonAncestor;
    chainA_1->nHeight = 101;
    chainA_1->nTime = commonAncestor->nTime + 60;
    chainA_1->nStakeModifier = 1000; // Higher stake weight
    
    CBlockIndex* chainA_2 = new CBlockIndex();
    chainA_2->pprev = chainA_1;
    chainA_2->nHeight = 102;
    chainA_2->nTime = chainA_1->nTime + 60;
    chainA_2->nStakeModifier = 1000;
    
    // Chain B - lower stake weight
    CBlockIndex* chainB_1 = new CBlockIndex();
    chainB_1->pprev = commonAncestor;
    chainB_1->nHeight = 101;
    chainB_1->nTime = commonAncestor->nTime + 60;
    chainB_1->nStakeModifier = 500; // Lower stake weight
    
    CBlockIndex* chainB_2 = new CBlockIndex();
    chainB_2->pprev = chainB_1;
    chainB_2->nHeight = 102;
    chainB_2->nTime = chainB_1->nTime + 60;
    chainB_2->nStakeModifier = 500;
    
    // Compare chains
    bool chainABetter = CompareChains(chainA_2, chainB_2);
    
    // Chain A should be selected (higher stake weight)
    BOOST_CHECK_EQUAL(chainABetter, true);
}

/* Test minimum stake enforcement */
BOOST_AUTO_TEST_CASE(minimum_stake_enforcement)
{
    // Create a stake transaction with insufficient amount
    CAmount insufficientStake = 31 * COIN; // Below 32 BTC minimum
    
    // Check if stake is valid
    bool insufficientResult = CheckStakeAmount(insufficientStake);
    BOOST_CHECK_EQUAL(insufficientResult, false);
    
    // Create a stake transaction with sufficient amount
    CAmount sufficientStake = 32 * COIN; // Exactly 32 BTC minimum
    
    // Check if stake is valid
    bool sufficientResult = CheckStakeAmount(sufficientStake);
    BOOST_CHECK_EQUAL(sufficientResult, true);
    
    // Create a stake transaction with more than minimum
    CAmount largeStake = 100 * COIN; // 100 BTC
    
    // Check if stake is valid
    bool largeResult = CheckStakeAmount(largeStake);
    BOOST_CHECK_EQUAL(largeResult, true);
}

BOOST_AUTO_TEST_SUITE_END()
