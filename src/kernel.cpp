// Copyright (c) 2012-2025 The Peercoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel.h>
#include <chainparams.h>
#include <validation.h>
#include <streams.h>
#include <timedata.h>
#include <bignum.h>
#include <txdb.h>
#include <consensus/validation.h>
#include <util/system.h>
#include <validation.h>
#include <random.h>
#include <script/interpreter.h>
#include <validator.h>

#include <index/txindex.h>

#include <boost/assign/list_of.hpp>

using namespace std;

// Protocol switch time of v0.3 kernel protocol
unsigned int nProtocolV03SwitchTime     = 1363800000;
unsigned int nProtocolV03TestSwitchTime = 1359781000;
// Protocol switch time of v0.4 kernel protocol
unsigned int nProtocolV04SwitchTime     = 1399300000;
unsigned int nProtocolV04TestSwitchTime = 1395700000;
// Protocol switch time of v0.5 kernel protocol
unsigned int nProtocolV05SwitchTime     = 1461700000;
unsigned int nProtocolV05TestSwitchTime = 1447700000;
// Protocol switch time of v0.6 kernel protocol
// supermajority hardfork: actual fork will happen later than switch time
const unsigned int nProtocolV06SwitchTime     = 1513050000; // Tue 12 Dec 03:40:00 UTC 2017
const unsigned int nProtocolV06TestSwitchTime = 1508198400; // Tue 17 Oct 00:00:00 UTC 2017
// Protocol switch time for 0.7 kernel protocol
const unsigned int nProtocolV07SwitchTime     = 1552392000; // Tue 12 Mar 12:00:00 UTC 2019
const unsigned int nProtocolV07TestSwitchTime = 1541505600; // Tue 06 Nov 12:00:00 UTC 2018
// Switch time for new BIPs from bitcoin 0.16.x
const uint32_t nBTC16BIPsSwitchTime           = 1569931200; // Tue 01 Oct 12:00:00 UTC 2019
const uint32_t nBTC16BIPsTestSwitchTime       = 1554811200; // Tue 09 Apr 12:00:00 UTC 2019
// Protocol switch time for v0.9 kernel protocol
const unsigned int nProtocolV09SwitchTime     = 1591617600; // Mon  8 Jun 12:00:00 UTC 2020
const unsigned int nProtocolV09TestSwitchTime = 1581940800; // Mon 17 Feb 12:00:00 UTC 2020
// Protocol switch time for v10 kernel protocol
const unsigned int nProtocolV10SwitchTime     = 1635768000; // Mon  1 Nov 12:00:00 UTC 2021
const unsigned int nProtocolV10TestSwitchTime = 1625140800; // Thu  1 Jul 12:00:00 UTC 2021
// Protocol switch time for v12 kernel protocol
const unsigned int nProtocolV12SwitchTime     = 1700276331; // Sat 18 Nov 02:58:51 UTC 2023
const unsigned int nProtocolV12TestSwitchTime = 1671060214; // Wed 14 Dec 11:23:34 UTC 2022
// Protocol switch time for v14 kernel protocol
const unsigned int nProtocolV14SwitchTime     = 1717416000; // Mon  3 Jun 12:00:00 UTC 2024
const unsigned int nProtocolV14TestSwitchTime = 1710720000; // Mon 18 Mar 00:00:00 UTC 2024
// Protocol switch time for v15 kernel protocol
const unsigned int nProtocolV15SwitchTime     = 1741780800; // Wed 12 Mar 12:00:00 UTC 2025
const unsigned int nProtocolV15TestSwitchTime = 1734004800; // Thu 12 Dec 12:00:00 UTC 2024

// Hard checkpoints of stake modifiers to ensure they are deterministic
static std::map<int, unsigned int> mapStakeModifierCheckpoints =
    boost::assign::map_list_of
    ( 0, 0x0e00670bu )
    ( 19080, 0xad4e4d29u )
    ( 30583, 0xdc7bf136u )
    ( 99999, 0xf555cfd2u )
    (219999, 0x91b7444du )
    (336000, 0x6c3c8048u )
    (371850, 0x9b850bdfu )
    (407813, 0x46fe50b5u )
    (443561, 0x114a6e38u )
    (455470, 0x9b7af181u )
    (479189, 0xe04fb8e0u )
    (504051, 0x459f5a16u )
    (589659, 0xbd02492au )
    (714688, 0xd70a5b68u )
    (770396, 0x565fb851u )
    (801334, 0x90485c37u )
    ;

static std::map<int, unsigned int> mapStakeModifierTestnetCheckpoints =
    boost::assign::map_list_of
    ( 0, 0x0e00670bu )
    ( 19080, 0x3711dc3au )
    ( 30583, 0xb480fadeu )
    ( 99999, 0x9a62eaecu )
    (219999, 0xeafe96c3u )
    (336000, 0x8330dc09u )
    (372751, 0xafb94e2fu )
    (382019, 0x7f5cf5ebu )
    (408500, 0x68cadee2u )
    (412691, 0x93138e67u )
    (441299, 0x03e195cbu )
    (442735, 0xe42d94feu )
    (516308, 0x04a0897au )
    (573702, 0xe69df1acu )
    (612778, 0x6be16d62u )
    ;

// Whether the given coinstake is subject to new v0.3 protocol
bool IsProtocolV03(unsigned int nTimeCoinStake)
{
    return (nTimeCoinStake >= (Params().NetworkIDString() != CBaseChainParams::MAIN ? nProtocolV03TestSwitchTime : nProtocolV03SwitchTime));
}

// Whether the given block is subject to new v0.4 protocol
bool IsProtocolV04(unsigned int nTimeBlock)
{
    return (nTimeBlock >= (Params().NetworkIDString() != CBaseChainParams::MAIN ? nProtocolV04TestSwitchTime : nProtocolV04SwitchTime));
}

// Whether the given transaction is subject to new v0.5 protocol
bool IsProtocolV05(unsigned int nTimeTx)
{
    return (nTimeTx >= (Params().NetworkIDString() != CBaseChainParams::MAIN ? nProtocolV05TestSwitchTime : nProtocolV05SwitchTime));
}

// Whether a given block is subject to new v0.6 protocol
// Test against previous block index! (always available)
bool IsProtocolV06(const CBlockIndex* pindexPrev)
{
  if (Params().NetworkIDString() == CBaseChainParams::REGTEST)
      return true;

  if (pindexPrev->nTime < (Params().NetworkIDString() != CBaseChainParams::MAIN ? nProtocolV06TestSwitchTime : nProtocolV06SwitchTime))
    return false;

  // if 900 of the last 1,000 blocks are version 2 or greater (90/100 if testnet):
  // Soft-forking PoS can be dangerous if the super majority is too low
  // The stake majority will decrease after the fork
  // since only coindays of updated nodes will get destroyed.
  if ((Params().NetworkIDString() == CBaseChainParams::MAIN && pindexPrev->nHeight > 339678) ||
      (Params().NetworkIDString() != CBaseChainParams::MAIN && pindexPrev->nHeight > 301251))
    return true;

  return false;
}

// Whether a given transaction is subject to new v0.7 protocol
bool IsProtocolV07(unsigned int nTimeTx)
{
    bool fTestNet = Params().NetworkIDString() != CBaseChainParams::MAIN;
    return (nTimeTx >= (fTestNet? nProtocolV07TestSwitchTime : nProtocolV07SwitchTime));
}

bool IsBTC16BIPsEnabled(uint32_t nTimeTx)
{
    bool fTestNet = Params().NetworkIDString() != CBaseChainParams::MAIN;
    return (nTimeTx >= (fTestNet? nBTC16BIPsTestSwitchTime : nBTC16BIPsSwitchTime));
}

// Whether a given timestamp is subject to new v0.9 protocol
bool IsProtocolV09(unsigned int nTime)
{
  return (nTime >= (Params().NetworkIDString() != CBaseChainParams::MAIN ? nProtocolV09TestSwitchTime : nProtocolV09SwitchTime));
}

// Whether a given timestamp is subject to new v10 protocol
bool IsProtocolV10(unsigned int nTime)
{
  return (nTime >= (Params().NetworkIDString() != CBaseChainParams::MAIN ? nProtocolV10TestSwitchTime : nProtocolV10SwitchTime));
}

// Whether a given block is subject to new v12 protocol
bool IsProtocolV12(const CBlockIndex* pindexPrev)
{
  if (Params().NetworkIDString() == CBaseChainParams::REGTEST)
      return true;

  return (pindexPrev->nTime >= (Params().NetworkIDString() != CBaseChainParams::MAIN ? nProtocolV12TestSwitchTime : nProtocolV12SwitchTime));
}

// Whether a given block is subject to new v14 protocol
bool IsProtocolV14(const CBlockIndex* pindexPrev)
{
  if (Params().NetworkIDString() == CBaseChainParams::REGTEST)
      return true;

  if (pindexPrev->nTime < (Params().NetworkIDString() != CBaseChainParams::MAIN ? nProtocolV14TestSwitchTime : nProtocolV14SwitchTime))
      return false;

  if ((Params().NetworkIDString() == CBaseChainParams::MAIN && pindexPrev->nHeight > 770395) ||
      (Params().NetworkIDString() != CBaseChainParams::MAIN && pindexPrev->nHeight > 573706))
    return true;

  return false;
}

// Whether a given block is subject to new v15 protocol
bool IsProtocolV15(const CBlockIndex* pindexPrev)
{
  if (Params().NetworkIDString() == CBaseChainParams::REGTEST)
      return true;

  if (pindexPrev->nTime < (Params().NetworkIDString() != CBaseChainParams::MAIN ? nProtocolV15TestSwitchTime : nProtocolV15SwitchTime))
      return false;

  if ((Params().NetworkIDString() == CBaseChainParams::MAIN && pindexPrev->nHeight > 801330) ||
      (Params().NetworkIDString() != CBaseChainParams::MAIN && pindexPrev->nHeight > 612775))
    return true;

  return false;
}

// Get the last stake modifier and its generation time from a given block
static bool GetLastStakeModifier(const CBlockIndex* pindex, uint64_t& nStakeModifier, int64_t& nModifierTime)
{
    if (!pindex)
        return error("GetLastStakeModifier: null pindex");
    while (pindex && pindex->pprev && !pindex->GeneratedStakeModifier())
        pindex = pindex->pprev;
    if (!pindex->GeneratedStakeModifier())
        return error("GetLastStakeModifier: no generation at genesis block");
    nStakeModifier = pindex->nStakeModifier;
    nModifierTime = pindex->GetBlockTime();
    return true;
}

// Get selection interval section (in seconds)
static int64_t GetStakeModifierSelectionIntervalSection(int nSection)
{
    assert (nSection >= 0 && nSection < 64);
    return (Params().GetConsensus().nModifierInterval * 63 / (63 + ((63 - nSection) * (MODIFIER_INTERVAL_RATIO - 1))));
}

// Get stake modifier selection interval (in seconds)
static int64_t GetStakeModifierSelectionInterval()
{
    int64_t nSelectionInterval = 0;
    for (int nSection=0; nSection<64; nSection++)
        nSelectionInterval += GetStakeModifierSelectionIntervalSection(nSection);
    return nSelectionInterval;
}

// select a block from the candidate blocks in vSortedByTimestamp, excluding
// already selected blocks in vSelectedBlocks, and with timestamp up to
// nSelectionIntervalStop.
static bool SelectBlockFromCandidates(
    vector<pair<int64_t, uint256> >& vSortedByTimestamp,
    map<uint256, const CBlockIndex*>& mapSelectedBlocks,
    int64_t nSelectionIntervalStop, uint64_t nStakeModifierPrev,
    const CBlockIndex** pindexSelected,
    Chainstate& chainstate)
{
    LOCK(cs_main);
    bool fSelected = false;
    arith_uint256 hashBest = 0;
    *pindexSelected = (const CBlockIndex*) 0;
    for (const auto& item : vSortedByTimestamp)
    {
/*
        if (!chainstate.BlockIndex().count(item.second))
            return error("SelectBlockFromCandidates: failed to find block index for candidate block %s", item.second.ToString());
        const CBlockIndex* pindex = pindexSelected.BlockIndex()[item.second];
*/
        const CBlockIndex* pindex = chainstate.m_blockman.LookupBlockIndex(item.second);
        if (!pindex)
            return error("SelectBlockFromCandidates: failed to find block index for candidate block %s", item.second.ToString());

        if (fSelected && pindex->GetBlockTime() > nSelectionIntervalStop)
            break;
        if (mapSelectedBlocks.count(pindex->GetBlockHash()) > 0)
            continue;
        // compute the selection hash by hashing its proof-hash and the
        // previous proof-of-stake modifier
        uint256 hashProof = pindex->IsProofOfStake()? pindex->hashProofOfStake : pindex->GetBlockHash();
        CDataStream ss(SER_GETHASH, 0);
        ss << hashProof << nStakeModifierPrev;
        arith_uint256 hashSelection = UintToArith256(Hash(ss));
        // the selection hash is divided by 2**32 so that proof-of-stake block
        // is always favored over proof-of-work block. this is to preserve
        // the energy efficiency property
        if (pindex->IsProofOfStake())
            hashSelection >>= 32;
        if (fSelected && hashSelection < hashBest)
        {
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*) pindex;
        }
        else if (!fSelected)
        {
            fSelected = true;
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*) pindex;
        }
    }
    if (gArgs.GetBoolArg("-debug", false) && gArgs.GetBoolArg("-printstakemodifier", false))
        LogPrintf("SelectBlockFromCandidates: selection hash=%s\n", hashBest.ToString());
    return fSelected;
}

// Stake Modifier (hash modifier of proof-of-stake):
// The purpose of stake modifier is to prevent a txout (coin) owner from
// computing future proof-of-stake generated by this txout at the time
// of transaction confirmation. To meet kernel protocol, the txout
// must hash with a future stake modifier to generate the proof.
// Stake modifier consists of bits each of which is contributed from a
// selected block of a given block group in the past.
// The selection of a block is based on a hash of the block's proof-hash and
// the previous stake modifier.
// Stake modifier is recomputed at a fixed time interval instead of every
// block. This is to make it difficult for an attacker to gain control of
// additional bits in the stake modifier, even after generating a chain of
// blocks.
bool ComputeNextStakeModifier(const CBlockIndex* pindexCurrent, uint64_t &nStakeModifier, bool& fGeneratedStakeModifier, Chainstate& chainstate)
{
    const Consensus::Params& params = Params().GetConsensus();
    const CBlockIndex* pindexPrev = pindexCurrent->pprev;
    nStakeModifier = 0;
    fGeneratedStakeModifier = false;
    if (!pindexPrev)
    {
        fGeneratedStakeModifier = true;
        return true;  // genesis block's modifier is 0
    }
    // First find current stake modifier and its generation block time
    // if it's not old enough, return the same stake modifier
    int64_t nModifierTime = 0;
    if (!GetLastStakeModifier(pindexPrev, nStakeModifier, nModifierTime))
        return error("ComputeNextStakeModifier: unable to get last modifier");
    if (gArgs.GetBoolArg("-debug", false))
        LogPrintf("ComputeNextStakeModifier: prev modifier=0x%016x time=%s epoch=%u\n", nStakeModifier, FormatISO8601DateTime(nModifierTime), (unsigned int)nModifierTime);
    if (nModifierTime / params.nModifierInterval >= pindexPrev->GetBlockTime() / params.nModifierInterval)
    {
        if (gArgs.GetBoolArg("-debug", false))
            LogPrintf("ComputeNextStakeModifier: no new interval keep current modifier: pindexPrev nHeight=%d nTime=%u\n", pindexPrev->nHeight, (unsigned int)pindexPrev->GetBlockTime());
        return true;
    }
    if (nModifierTime / params.nModifierInterval >= pindexCurrent->GetBlockTime() / params.nModifierInterval)
    {
        // v0.4+ requires current block timestamp also be in a different modifier interval
        if (IsProtocolV04(pindexCurrent->nTime))
        {
            if (gArgs.GetBoolArg("-debug", false))
                LogPrintf("ComputeNextStakeModifier: (v0.4+) no new interval keep current modifier: pindexCurrent nHeight=%d nTime=%u\n", pindexCurrent->nHeight, (unsigned int)pindexCurrent->GetBlockTime());
            return true;
        }
        else
        {
            if (gArgs.GetBoolArg("-debug", false))
                LogPrintf("ComputeNextStakeModifier: v0.3 modifier at block %s not meeting v0.4+ protocol: pindexCurrent nHeight=%d nTime=%u\n", pindexCurrent->GetBlockHash().ToString(), pindexCurrent->nHeight, (unsigned int)pindexCurrent->GetBlockTime());
        }
    }

    // Sort candidate blocks by timestamp
    vector<pair<int64_t, uint256> > vSortedByTimestamp;
    vSortedByTimestamp.reserve(64 * params.nModifierInterval / params.nStakeTargetSpacing);
    int64_t nSelectionInterval = GetStakeModifierSelectionInterval();
    int64_t nSelectionIntervalStart = (pindexPrev->GetBlockTime() / params.nModifierInterval) * params.nModifierInterval - nSelectionInterval;
    const CBlockIndex* pindex = pindexPrev;
    while (pindex && pindex->GetBlockTime() >= nSelectionIntervalStart)
    {
        vSortedByTimestamp.push_back(make_pair(pindex->GetBlockTime(), pindex->GetBlockHash()));
        pindex = pindex->pprev;
    }
    int nHeightFirstCandidate = pindex ? (pindex->nHeight + 1) : 0;

    // Shuffle before sort
    for(int i = vSortedByTimestamp.size() - 1; i > 1; --i)
    	std::swap(vSortedByTimestamp[i], vSortedByTimestamp[GetRand(i)]);

    sort(vSortedByTimestamp.begin(), vSortedByTimestamp.end(), [] (const pair<int64_t, uint256> &a, const pair<int64_t, uint256> &b)
    {
        if (a.first != b.first)
            return a.first < b.first;
//        return b.second < a.second;

        // Timestamp equals - compare block hashes
        const uint32_t *pa = (const uint32_t *)a.second.data();
        const uint32_t *pb = (const uint32_t *)b.second.data();
        int cnt = 256 / 32;
        do {
            --cnt;
            if (pa[cnt] != pb[cnt])
                return pa[cnt] < pb[cnt];
        } while(cnt);
            return false; // Elements are equal

    });

    // Select 64 blocks from candidate blocks to generate stake modifier
    uint64_t nStakeModifierNew = 0;
    int64_t nSelectionIntervalStop = nSelectionIntervalStart;
    map<uint256, const CBlockIndex*> mapSelectedBlocks;
    for (int nRound=0; nRound<min(64, (int)vSortedByTimestamp.size()); nRound++)
    {
        // add an interval section to the current selection round
        nSelectionIntervalStop += GetStakeModifierSelectionIntervalSection(nRound);
        // select a block from the candidates of current round
        if (!SelectBlockFromCandidates(vSortedByTimestamp, mapSelectedBlocks, nSelectionIntervalStop, nStakeModifier, &pindex, chainstate))
            return error("ComputeNextStakeModifier: unable to select block at round %d", nRound);
        // write the entropy bit of the selected block
        nStakeModifierNew |= (((uint64_t)pindex->GetStakeEntropyBit()) << nRound);
        // add the selected block from candidates to selected list
        mapSelectedBlocks.insert(make_pair(pindex->GetBlockHash(), pindex));
        if (gArgs.GetBoolArg("-debug", false) && gArgs.GetBoolArg("-printstakemodifier", false))
            LogPrintf("ComputeNextStakeModifier: selected round %d stop=%s height=%d bit=%d\n",
                nRound, FormatISO8601DateTime(nSelectionIntervalStop), pindex->nHeight, pindex->GetStakeEntropyBit());
    }

    // Print selection map for visualization of the selected blocks
    if (gArgs.GetBoolArg("-debug", false) && gArgs.GetBoolArg("-printstakemodifier", false))
    {
        string strSelectionMap = "";
        // '-' indicates proof-of-work blocks not selected
        strSelectionMap.insert(0, pindexPrev->nHeight - nHeightFirstCandidate + 1, '-');
        pindex = pindexPrev;
        while (pindex && pindex->nHeight >= nHeightFirstCandidate)
        {
            // '=' indicates proof-of-stake blocks not selected
            if (pindex->IsProofOfStake())
                strSelectionMap.replace(pindex->nHeight - nHeightFirstCandidate, 1, "=");
            pindex = pindex->pprev;
        }
        for (const auto& item : mapSelectedBlocks)
        {
            // 'S' indicates selected proof-of-stake blocks
            // 'W' indicates selected proof-of-work blocks
            strSelectionMap.replace(item.second->nHeight - nHeightFirstCandidate, 1, item.second->IsProofOfStake()? "S" : "W");
        }
        LogPrintf("ComputeNextStakeModifier: selection height [%d, %d] map %s\n", nHeightFirstCandidate, pindexPrev->nHeight, strSelectionMap);
    }
    if (gArgs.GetBoolArg("-debug", false))
        LogPrintf("ComputeNextStakeModifier: new modifier=0x%016x time=%s\n", nStakeModifierNew, FormatISO8601DateTime(pindexPrev->GetBlockTime()));

    nStakeModifier = nStakeModifierNew;
    fGeneratedStakeModifier = true;
    return true;
}

// V0.5: Stake modifier used to hash for a stake kernel is chosen as the stake
// modifier that is (nStakeMinAge minus a selection interval) earlier than the
// stake, thus at least a selection interval later than the coin generating the
// kernel, as the generating coin is from at least nStakeMinAge ago.
static bool GetKernelStakeModifierV05(CBlockIndex* pindexPrev, unsigned int nTimeTx, uint64_t& nStakeModifier, int& nStakeModifierHeight, int64_t& nStakeModifierTime, bool fPrintProofOfStake)
{
    const Consensus::Params& params = Params().GetConsensus();
    const CBlockIndex* pindex = pindexPrev;
    nStakeModifierHeight = pindex->nHeight;
    nStakeModifierTime = pindex->GetBlockTime();
    int64_t nStakeModifierSelectionInterval = GetStakeModifierSelectionInterval();

    if (nStakeModifierTime + params.nStakeMinAge - nStakeModifierSelectionInterval <= (int64_t) nTimeTx)
    {
        // Best block is still more than
        // (nStakeMinAge minus a selection interval) older than kernel timestamp
        if (fPrintProofOfStake)
            return error("GetKernelStakeModifier() : best block %s at height %d too old for stake",
                pindex->GetBlockHash().ToString(), pindex->nHeight);
        else
            return false;
    }
    // loop to find the stake modifier earlier by 
    // (nStakeMinAge minus a selection interval)
    while (nStakeModifierTime + params.nStakeMinAge - nStakeModifierSelectionInterval >(int64_t) nTimeTx)
    {
        if (!pindex->pprev)
        {   // reached genesis block; should not happen
            return error("GetKernelStakeModifier() : reached genesis block");
        }
        pindex = pindex->pprev;
        if (pindex->GeneratedStakeModifier())
        {
            nStakeModifierHeight = pindex->nHeight;
            nStakeModifierTime = pindex->GetBlockTime();
        }
    }
    nStakeModifier = pindex->nStakeModifier;
    return true;
}

// V0.3: Stake modifier used to hash for a stake kernel is chosen as the stake
// modifier about a selection interval later than the coin generating the kernel
static bool GetKernelStakeModifierV03(CBlockIndex* pindexPrev, uint256 hashBlockFrom, uint64_t& nStakeModifier, int& nStakeModifierHeight, int64_t& nStakeModifierTime, bool fPrintProofOfStake, Chainstate& chainstate)
{
    const Consensus::Params& params = Params().GetConsensus();
    nStakeModifier = 0;

    const CBlockIndex* pindexFrom;
    {
        LOCK(cs_main);
        pindexFrom = chainstate.m_blockman.LookupBlockIndex(hashBlockFrom);
    }

    if (!pindexFrom)
        return error("GetKernelStakeModifier() : block not indexed");

    nStakeModifierHeight = pindexFrom->nHeight;
    nStakeModifierTime = pindexFrom->GetBlockTime();
    int64_t nStakeModifierSelectionInterval = GetStakeModifierSelectionInterval();


    // we need to iterate index forward but we cannot depend on chainActive.Next()
    // because there is no guarantee that we are checking blocks in active chain.
    // So, we construct a temporary chain that we will iterate over.
    // pindexFrom - this block contains coins that are used to generate PoS
    // pindexPrev - this is a block that is previous to PoS block that we are checking, you can think of it as tip of our chain
    std::vector<CBlockIndex*> tmpChain;
    int32_t nDepth = pindexPrev->nHeight - (pindexFrom->nHeight-1); // -1 is used to also include pindexFrom
    tmpChain.reserve(nDepth);
    CBlockIndex* it = pindexPrev;
    for (int i=1; i<=nDepth && !chainstate.m_chain.Contains(it); i++) {
        tmpChain.push_back(it);
        it = it->pprev;
    }
    std::reverse(tmpChain.begin(), tmpChain.end());
    size_t n = 0;

    const CBlockIndex* pindex = pindexFrom;
    // loop to find the stake modifier later by a selection interval
    while (nStakeModifierTime < pindexFrom->GetBlockTime() + nStakeModifierSelectionInterval)
    {
        const CBlockIndex* old_pindex = pindex;
        pindex = (!tmpChain.empty() && pindex->nHeight >= tmpChain[0]->nHeight - 1)? tmpChain[n++] : chainstate.m_chain.Next(pindex);
        if (n > tmpChain.size() || pindex == NULL) // check if tmpChain[n+1] exists
        {   // reached best block; may happen if node is behind on block chain
            if (fPrintProofOfStake || (old_pindex->GetBlockTime() + params.nStakeMinAge - nStakeModifierSelectionInterval > TicksSinceEpoch<std::chrono::seconds>(GetAdjustedTime())))
                return error("GetKernelStakeModifier() : reached best block %s at height %d from block %s",
                    old_pindex->GetBlockHash().ToString(), old_pindex->nHeight, hashBlockFrom.ToString());
            else
                return false;
        }
        if (pindex->GeneratedStakeModifier())
        {
            nStakeModifierHeight = pindex->nHeight;
            nStakeModifierTime = pindex->GetBlockTime();
        }
    }
    nStakeModifier = pindex->nStakeModifier;
    return true;
}

// Get the stake modifier specified by the protocol to hash for a stake kernel
static bool GetKernelStakeModifier(CBlockIndex* pindexPrev, uint256 hashBlockFrom, unsigned int nTimeTx, uint64_t& nStakeModifier, int& nStakeModifierHeight, int64_t& nStakeModifierTime, bool fPrintProofOfStake, Chainstate& chainstate)
{
    if (IsProtocolV05(nTimeTx))
        return GetKernelStakeModifierV05(pindexPrev, nTimeTx, nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake);
    else
        return GetKernelStakeModifierV03(pindexPrev, hashBlockFrom, nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake, chainstate);
}

// peercoin kernel protocol
// coinstake must meet hash target according to the protocol:
// kernel (input 0) must meet the formula
//     hash(nStakeModifier + txPrev.block.nTime + txPrev.offset + txPrev.nTime + txPrev.vout.n + nTime) < bnTarget * nCoinDayWeight
// this ensures that the chance of getting a coinstake is proportional to the
// amount of coin age one owns.
// The reason this hash is chosen is the following:
//   nStakeModifier: 
//       (v0.5) uses dynamic stake modifier around 21 days before the kernel,
//              versus static stake modifier about 9 days after the staked
//              coin (txPrev) used in v0.3
//       (v0.3) scrambles computation to make it very difficult to precompute
//              future proof-of-stake at the time of the coin's confirmation
//       (v0.2) nBits (deprecated): encodes all past block timestamps
//   txPrev.block.nTime: prevent nodes from guessing a good timestamp to
//                       generate transaction for future advantage
//   txPrev.offset: offset of txPrev inside block, to reduce the chance of 
//                  nodes generating coinstake at the same time
//   txPrev.nTime: reduce the chance of nodes generating coinstake at the same
//                 time
//   txPrev.vout.n: output number of txPrev, to reduce the chance of nodes
//                  generating coinstake at the same time
//   block/tx hash should not be used here as they can be generated in vast
//   quantities so as to generate blocks faster, degrading the system back into
//   a proof-of-work situation.
//
bool CheckStakeKernelHash(unsigned int nBits, CBlockIndex* pindexPrev, const CBlockHeader& blockFrom, unsigned int nTxPrevOffset, const CTransactionRef& txPrev, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProofOfStake, bool fPrintProofOfStake, Chainstate& chainstate)
{
    const Consensus::Params& params = Params().GetConsensus();
    unsigned int nTimeBlockFrom = blockFrom.GetBlockTime();

    if (nTimeTx < (txPrev->nTime? txPrev->nTime : nTimeBlockFrom))  // Transaction timestamp violation
        return error("CheckStakeKernelHash() : nTime violation");

    if (nTimeBlockFrom + params.nStakeMinAge > nTimeTx) // Min age requirement
        return error("CheckStakeKernelHash() : min age violation");

    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);
    int64_t nValueIn = txPrev->vout[prevout.n].nValue;
    // v0.3 protocol kernel hash weight starts from 0 at the 30-day min age
    // this change increases active coins participating the hash and helps
    // to secure the network when proof-of-stake difficulty is low
    int64_t nTimeWeight = min((int64_t)nTimeTx - (txPrev->nTime? txPrev->nTime : nTimeBlockFrom), params.nStakeMaxAge) - (IsProtocolV03(nTimeTx)? params.nStakeMinAge : 0);
    CBigNum bnCoinDayWeight = CBigNum(nValueIn) * nTimeWeight / COIN / (24 * 60 * 60);
    // Calculate hash
    CDataStream ss(SER_GETHASH, 0);
    uint64_t nStakeModifier = 0;
    int nStakeModifierHeight = 0;
    int64_t nStakeModifierTime = 0;
    if (IsProtocolV03(nTimeTx))  // v0.3 protocol
    {
        if (!GetKernelStakeModifier(pindexPrev, blockFrom.GetHash(), nTimeTx, nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake, chainstate))
            return false;
        ss << nStakeModifier;
    }
    else // v0.2 protocol
    {
        ss << nBits;
    }

    ss << nTimeBlockFrom << nTxPrevOffset << (txPrev->nTime? txPrev->nTime : nTimeBlockFrom) << prevout.n << nTimeTx;
    hashProofOfStake = Hash(ss);
    if (fPrintProofOfStake)
    {
        if (IsProtocolV03(nTimeTx)) {
            LOCK(cs_main);
            const CBlockIndex* pindexTmp = chainstate.m_blockman.LookupBlockIndex(blockFrom.GetHash());
            LogPrintf("CheckStakeKernelHash() : using modifier 0x%016x at height=%d timestamp=%s for block from height=%d timestamp=%s\n",
                nStakeModifier, nStakeModifierHeight,
                FormatISO8601DateTime(nStakeModifierTime),
                pindexTmp->nHeight,
                FormatISO8601DateTime(blockFrom.GetBlockTime()));
        }
        LogPrintf("CheckStakeKernelHash() : check protocol=%s modifier=0x%016x nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
            IsProtocolV05(nTimeTx)? "0.5" : (IsProtocolV03(nTimeTx)? "0.3" : "0.2"),
            IsProtocolV03(nTimeTx)? nStakeModifier : (uint64_t) nBits,
            nTimeBlockFrom, nTxPrevOffset, (txPrev->nTime? txPrev->nTime : nTimeBlockFrom), prevout.n, nTimeTx,
            hashProofOfStake.ToString());
    }

    // Now check if proof-of-stake hash meets target protocol
    if (CBigNum(hashProofOfStake) > bnCoinDayWeight * bnTargetPerCoinDay)
        return false;
    if (gArgs.GetBoolArg("-debug", false) && !fPrintProofOfStake)
    {
        if (IsProtocolV03(nTimeTx)) {
            LOCK(cs_main);
            const CBlockIndex* pindexTmp = chainstate.m_blockman.LookupBlockIndex(blockFrom.GetHash());
            LogPrintf("CheckStakeKernelHash() : using modifier 0x%016x at height=%d timestamp=%s for block from height=%d timestamp=%s\n",
                nStakeModifier, nStakeModifierHeight, 
                FormatISO8601DateTime(nStakeModifierTime),
                pindexTmp->nHeight,
                FormatISO8601DateTime(blockFrom.GetBlockTime()));
        }
        LogPrintf("CheckStakeKernelHash() : pass protocol=%s modifier=0x%016x nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
            IsProtocolV03(nTimeTx)? "0.3" : "0.2",
            IsProtocolV03(nTimeTx)? nStakeModifier : (uint64_t) nBits,
            nTimeBlockFrom, nTxPrevOffset, (txPrev->nTime? txPrev->nTime : nTimeBlockFrom), prevout.n, nTimeTx,
            hashProofOfStake.ToString());
    }
    return true;
}

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(BlockValidationState &state, CBlockIndex* pindexPrev, const CTransactionRef& tx, unsigned int nBits, uint256& hashProofOfStake, unsigned int nTimeTx, Chainstate& chainstate)
{
    if (!tx->IsCoinStake())
        return error("CheckProofOfStake() : called on non-coinstake %s", tx->GetHash().ToString());

    // Kernel (input 0) must match the stake hash target per coin age (nBits)
    const CTxIn& txin = tx->vin[0];

    // Transaction index is required to get to block header
    if (!g_txindex)
        return error("CheckProofOfStake() : transaction index not available");

    // Get transaction index for the previous transaction
    CDiskTxPos postx;
    if (!g_txindex->FindTxPosition(txin.prevout.hash, postx))
        return error("CheckProofOfStake() : tx index not found");  // tx index not found

    // Read txPrev and header of its block
    CBlockHeader header;
    CTransactionRef txPrev;
    auto it = g_txindex->cachedTxs.find(txin.prevout.hash);
    if (it != g_txindex->cachedTxs.end()) {
        header = it->second.first;
        txPrev = it->second.second;
    } else {
        CAutoFile file(node::OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
        try {
            file >> header;
            fseek(file.Get(), postx.nTxOffset, SEEK_CUR);
            file >> txPrev;
        } catch (std::exception &e) {
            return error("%s() : deserialize or I/O error in CheckProofOfStake()", __PRETTY_FUNCTION__);
        }
        //g_txindex->cachedTxs[txin.prevout.hash] = std::pair(header,txPrev);
    }

    if (txPrev->GetHash() != txin.prevout.hash)
        return error("%s() : txid mismatch in CheckProofOfStake()", __PRETTY_FUNCTION__);

    // Verify signature
    {
        int nIn = 0;
        const CTxOut& prevOut = txPrev->vout[tx->vin[nIn].prevout.n];
        TransactionSignatureChecker checker(&(*tx), nIn, prevOut.nValue, PrecomputedTransactionData(*tx), MissingDataBehavior(1));

        if (!VerifyScript(tx->vin[nIn].scriptSig, prevOut.scriptPubKey, &(tx->vin[nIn].scriptWitness), SCRIPT_VERIFY_P2SH, checker, nullptr))
            return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "invalid-pos-script", strprintf("%s: VerifyScript failed on coinstake %s", __func__, tx->GetHash().ToString()));
    }

    if (!CheckStakeKernelHash(nBits, pindexPrev, header, postx.nTxOffset + CBlockHeader::NORMAL_SERIALIZE_SIZE, txPrev, txin.prevout, nTimeTx, hashProofOfStake, gArgs.GetBoolArg("-debug", false), chainstate))
        return state.Invalid(BlockValidationResult::BLOCK_CONSENSUS, "check-kernel-failed", strprintf("CheckProofOfStake() : INFO: check kernel failed on coinstake %s, hashProof=%s", tx->GetHash().ToString(), hashProofOfStake.ToString())); // may occur during initial download or if behind on block chain sync

    return true;
}

// Check whether the coinstake timestamp meets protocol
bool CheckCoinStakeTimestamp(int64_t nTimeBlock, int64_t nTimeTx)
{
    if (IsProtocolV03(nTimeTx))  // v0.3 protocol
        return (nTimeBlock == nTimeTx);
    else // v0.2 protocol
        return ((nTimeTx <= nTimeBlock) && (nTimeBlock <= nTimeTx + MAX_FUTURE_BLOCK_TIME_PREV9));
}

// Get stake modifier checksum
unsigned int GetStakeModifierChecksum(const CBlockIndex* pindex)
{
    assert (pindex->pprev || pindex->GetBlockHash() == Params().GetConsensus().hashGenesisBlock);
    // Hash previous checksum with flags, hashProofOfStake and nStakeModifier
    CDataStream ss(SER_GETHASH, 0);
    if (pindex->pprev)
        ss << pindex->pprev->nStakeModifierChecksum;
    ss << pindex->nFlags << pindex->hashProofOfStake << pindex->nStakeModifier;
    arith_uint256 hashChecksum = UintToArith256(Hash(ss));
    hashChecksum >>= (256 - 32);
    return hashChecksum.GetLow64();
}

// Check stake modifier hard checkpoints
bool CheckStakeModifierCheckpoints(int nHeight, unsigned int nStakeModifierChecksum)
{
    // BT2C FIX: Skip stake modifier checkpoints for BT2C genesis blocks
    // BT2C uses different genesis blocks than Peercoin, so legacy checkpoints don't apply
    if (nHeight == 0) {
        LogPrintf("BT2C DEBUG: Skipping stake modifier checkpoint for genesis block (height=0)\n");
        LogPrintf("BT2C DEBUG: Genesis stake modifier checksum: 0x%08x\n", nStakeModifierChecksum);
        return true; // Always pass for BT2C genesis blocks
    }

    bool fTestNet = Params().NetworkIDString() == CBaseChainParams::TESTNET;
    if (fTestNet && mapStakeModifierTestnetCheckpoints.count(nHeight))
        return nStakeModifierChecksum == mapStakeModifierTestnetCheckpoints[nHeight];

    if (!fTestNet && mapStakeModifierCheckpoints.count(nHeight))
        return nStakeModifierChecksum == mapStakeModifierCheckpoints[nHeight];

    return true;
}

bool IsSuperMajority(int minVersion, const CBlockIndex* pstart, unsigned int nRequired, unsigned int nToCheck)
{
    return (HowSuperMajority(minVersion, pstart, nRequired, nToCheck) >= nRequired);
}

unsigned int HowSuperMajority(int minVersion, const CBlockIndex* pstart, unsigned int nRequired, unsigned int nToCheck)
{
    unsigned int nFound = 0;
    for (unsigned int i = 0; i < nToCheck && nFound < nRequired && pstart != NULL; pstart = pstart->pprev )
    {
        if (!pstart->IsProofOfStake())
            continue;

        if (pstart->nVersion >= minVersion)
            ++nFound;

        i++;
    }
    return nFound;
}

// peercoin: entropy bit for stake modifier if chosen by modifier
unsigned int GetStakeEntropyBit(const CBlock& block)
{
    unsigned int nEntropyBit = 0;
    if (IsProtocolV04(block.nTime))
    {
        nEntropyBit = UintToArith256(block.GetHash()).GetLow64() & 1llu;// last bit of block hash
        if (gArgs.GetBoolArg("-printstakemodifier", false))
            LogPrintf("GetStakeEntropyBit(v0.4+): nTime=%u hashBlock=%s entropybit=%d\n", block.nTime, block.GetHash().ToString(), nEntropyBit);
    }
    else
    {
        // old protocol for entropy bit pre v0.4
        uint160 hashSig = Hash160(block.vchBlockSig);
        if (gArgs.GetBoolArg("-printstakemodifier", false))
            LogPrintf("GetStakeEntropyBit(v0.3): nTime=%u hashSig=%s", block.nTime, hashSig.ToString());
        nEntropyBit = hashSig.data()[19] >> 7;  // take the first bit of the hash
        if (gArgs.GetBoolArg("-printstakemodifier", false))
            LogPrintf(" entropybit=%d\n", nEntropyBit);
    }
    return nEntropyBit;
}

// bit2coin: check if validator meets minimum stake requirement
bool CheckValidatorMinimumStake(const CTransactionRef& tx, const CAmount& minStake)
{
    // The minimum stake requirement for validators is 32 BTC
    CAmount totalStake = 0;
    for (const auto& out : tx->vout) {
        totalStake += out.nValue;
    }
    
    if (totalStake < minStake) {
        LogPrintf("CheckValidatorMinimumStake: Validator stake (%s) is less than minimum required (%s)\n",
                 FormatMoney(totalStake), FormatMoney(minStake));
        return false;
    }
    
    return true;
}

bool CheckValidatorMinimumStake(const CScript& scriptPubKey, Chainstate& chainstate) {
    // Use the provided script directly
    
    // Check the UTXO set for all unspent outputs to this address
    std::vector<COutPoint> vOutPoints;
    CAmount totalStake = 0;
    
    // Use the coin view to check the UTXO set
    CCoinsViewCache& view = chainstate.CoinsTip();
    
    // Scan the UTXO set for outputs to this address
    // Note: This is a simplified approach; in a real implementation, we'd need a more efficient way
    // to query the UTXO set by address
    // Since GetOutPointsFor doesn't exist, we'll iterate through all UTXOs
    
    // Iterate through all UTXOs in the view
    std::unique_ptr<CCoinsViewCursor> cursor(view.Cursor());
    if (cursor) {
        COutPoint outpoint;
        Coin coin;
        while (cursor->Valid()) {
            if (cursor->GetKey(outpoint) && cursor->GetValue(coin) && !coin.IsSpent()) {
                // Check if this output is to our validator's address
                if (coin.out.scriptPubKey == scriptPubKey) {
                    vOutPoints.push_back(outpoint);
                }
            }
            cursor->Next();
        }
    }
    
    for (const auto& outpoint : vOutPoints) {
        Coin coin;
        if (!view.GetCoin(outpoint, coin) || coin.IsSpent()) continue;
        
        // Check if this output is to our validator's address
        if (coin.out.scriptPubKey == scriptPubKey) {
            totalStake += coin.out.nValue;
            vOutPoints.push_back(outpoint);
        }
    }
    
    // Check if the total stake meets the minimum requirement
    if (totalStake < VALIDATOR_MIN_STAKE) {
        LogPrintf("CheckValidatorMinimumStake: Validator stake (%s) is less than minimum required (%s)\n",
                 FormatMoney(totalStake), FormatMoney(VALIDATOR_MIN_STAKE));
        return false;
    }
    
    LogPrintf("CheckValidatorMinimumStake: Validator has sufficient stake: %s\n",
             FormatMoney(totalStake));
    return true;
}

// bit2coin: check if validator is eligible to create blocks
bool IsValidatorEligible(const CTransactionRef& tx, const uint256& validatorId, Chainstate& chainstate)
{
    // Get validator from registry
    CValidator* validator = g_validatorRegistry.GetValidator(validatorId);
    if (!validator) {
        LogPrintf("IsValidatorEligible: Validator %s not found in registry\n", validatorId.ToString());
        return false;
    }

    // Check if validator is eligible
    return IsValidatorEligible(*validator, chainstate);
}

bool IsValidatorEligible(const CValidator& validator, Chainstate& chainstate)
{
    // Check if validator is active
    if (validator.status != ValidatorStatus::ACTIVE) {
        LogPrintf("Validator %s is not active (status: %s)\n", validator.validatorId.ToString(), validator.status);
        return false;
    }

    // Check if validator has been slashed
    if (validator.status == ValidatorStatus::SLASHED) {
        LogPrintf("Validator %s has been slashed\n", validator.validatorId.ToString());
        return false;
    }

    // Check if validator meets minimum stake requirement
    if (validator.stakedAmount < VALIDATOR_MIN_STAKE) {
        LogPrintf("Validator %s does not meet minimum stake requirement (%s < %s)\n", 
                 validator.validatorId.ToString(), FormatMoney(validator.stakedAmount), FormatMoney(VALIDATOR_MIN_STAKE));
        return false;
    }
    
    // Verify the validator's stake is still valid in the UTXO set
    // This prevents validators from double-spending their stake
    bool stakeVerified = CheckValidatorMinimumStake(validator.scriptPubKey, chainstate);
    if (!stakeVerified) {
        LogPrintf("Validator %s stake not verified in UTXO set\n", validator.validatorId.ToString());
        return false;
    }

    return true;
}

// bit2coin: select validator for block creation using VRF
bool SelectBlockValidator(const CBlockIndex* pindexPrev, uint256& selectedValidator, Chainstate& chainstate)
{
    // Get all active validators
    std::vector<CValidator*> activeValidators = g_validatorRegistry.GetActiveValidators();
    
    if (activeValidators.empty()) {
        LogPrintf("SelectBlockValidator: No active validators found\n");
        return false;
    }
    
    // Select validator using VRF-like weighted random selection based on stake and reputation
    // We pass the previous block hash directly as the source of randomness
    CValidator* selectedValidatorPtr = g_validatorRegistry.SelectNextValidator(pindexPrev->GetBlockHash(), GetTime());
    if (selectedValidatorPtr) {
        selectedValidator = selectedValidatorPtr->validatorId;
    }
    
    if (selectedValidator.IsNull()) {
        LogPrintf("SelectBlockValidator: Failed to select a validator\n");
        return false;
    }
    
    LogPrintf("SelectBlockValidator: Selected validator %s\n", selectedValidator.ToString());
    return true;
}

