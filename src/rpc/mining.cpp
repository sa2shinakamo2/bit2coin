// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2022 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <chain.h>
#include <chainparams.h>
#include <consensus/amount.h>
#include <consensus/consensus.h>
#include <consensus/merkle.h>
#include <consensus/params.h>
#include <consensus/validation.h>
#include <core_io.h>
#include <deploymentinfo.h>
#include <key_io.h>
#include <interfaces/wallet.h>
#include <net.h>
#include <node/context.h>
#include <node/miner.h>
#include <pow.h>
#include <rpc/blockchain.h>
#include <rpc/mining.h>
#include <rpc/server.h>
#include <rpc/server_util.h>
#include <rpc/util.h>
#include <chain.h>
#include <script/descriptor.h>
#include <script/script.h>
#include <script/signingprovider.h>
#include <shutdown.h>
#include <timedata.h>
#include <txmempool.h>
#include <univalue.h>
#include <util/strencodings.h>
#include <util/string.h>
#include <util/system.h>
#include <util/translation.h>
#include <validation.h>
#include <validationinterface.h>
#include <warnings.h>

#include <wallet/rpc/util.h>
#include <wallet/rpc/wallet.h>
#include <wallet/wallet.h>

#include <kernel.h>

#include <memory>
#include <stdint.h>

using node::BlockAssembler;
using node::CBlockTemplate;
using node::NodeContext;
using node::RegenerateCommitments;
using node::UpdateTime;
using interfaces::WalletLoader;

using wallet::GetWalletForJSONRPCRequest;

/**
 * Return average network hashes per second based on the last 'lookup' blocks,
 * or from the last difficulty change if 'lookup' is nonpositive.
 * If 'height' is nonnegative, compute the estimate at the time when a given block was found.
 */
static UniValue GetNetworkHashPS(int lookup, int height, const CChain& active_chain) {
    const CBlockIndex* pb = active_chain.Tip();

    if (height >= 0 && height < active_chain.Height()) {
        pb = active_chain[height];
    }

    if (pb == nullptr || !pb->nHeight)
        return 0;

    //ppcTODO - redo this to fit peercoin
    // If lookup is -1, then use blocks since last difficulty change.
//    if (lookup <= 0)
//        lookup = pb->nHeight % Params().GetConsensus().DifficultyAdjustmentInterval() + 1;

    // If lookup is larger than chain, then set it to chain length.
    if (lookup > pb->nHeight)
        lookup = pb->nHeight;

    const CBlockIndex* pb0 = pb;
    int64_t minTime = pb0->GetBlockTime();
    int64_t maxTime = minTime;
    for (int i = 0; i < lookup; i++) {
        pb0 = pb0->pprev;
        int64_t time = pb0->GetBlockTime();
        minTime = std::min(time, minTime);
        maxTime = std::max(time, maxTime);
    }

    // In case there's a situation where minTime == maxTime, we don't want a divide by zero exception.
    if (minTime == maxTime)
        return 0;

    arith_uint256 workDiff = pb->nChainTrust - pb0->nChainTrust;
    int64_t timeDiff = maxTime - minTime;

    return workDiff.getdouble() / timeDiff;
}

static RPCHelpMan getnetworkhashps()
{
    return RPCHelpMan{"getnetworkhashps",
                "\nBT2C: Always returns 0 as BT2C is a PoS-only blockchain.\n"
                "\nIn PoW blockchains, this would return the estimated network hashes per second.\n",
                {
                    {"nblocks", RPCArg::Type::NUM, RPCArg::Default{120}, "Not used in BT2C."},
                    {"height", RPCArg::Type::NUM, RPCArg::Default{-1}, "Not used in BT2C."},
                },
                RPCResult{
                    RPCResult::Type::NUM, "", "Always 0 for BT2C"},
                RPCExamples{
                    HelpExampleCli("getnetworkhashps", "")
            + HelpExampleRpc("getnetworkhashps", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // BT2C: Always return 0 as this is a PoS-only blockchain
    return 0;
},
    };
}

// BT2C: get network Gh/s estimate (always returns 0 for PoS-only blockchain)
static RPCHelpMan getnetworkghps()
{
    return RPCHelpMan{"getnetworkghps",
                "\nReturns the estimated network Gh/s based on the last n blocks.\n"
                "For BT2C, this always returns 0 as it is a PoS-only blockchain.\n",
                {
                    {"nblocks", RPCArg::Type::NUM, RPCArg::Default{120}, "The number of blocks (ignored in BT2C)."},
                    {"height", RPCArg::Type::NUM, RPCArg::Default{-1}, "To estimate at the time of the given height (ignored in BT2C)."},
                },
                RPCResult{
                    RPCResult::Type::NUM, "", "Gigahashes per second estimated (always 0 for BT2C)"},
                RPCExamples{
                    HelpExampleCli("getnetworkghps", "")
            + HelpExampleRpc("getnetworkghps", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    // BT2C is a PoS-only blockchain, so network hashrate is always 0
    UniValue result(UniValue::VNUM);
    result.setInt(0);
    return result;
},
    };
}

// BT2C: Block generation for PoS-only blockchain
// Note: BT2C does not support PoW block generation, this function will always fail for PoW blocks
static bool GenerateBlock(ChainstateManager& chainman, CBlock& block, uint64_t& max_tries, std::shared_ptr<const CBlock>& block_out, bool process_new_block)
{
    block_out.reset();
    block.hashMerkleRoot = BlockMerkleRoot(block);
    
    // BT2C is PoS-only, so PoW block generation will always fail
    if (block.IsProofOfWork()) {
        LogPrintf("BT2C: PoW block generation attempted but BT2C is PoS-only\n");
        return false;
    }
    
    // For PoS blocks, we don't need to iterate through nonces
    if (max_tries == 0 || ShutdownRequested()) {
        return false;
    }

    block_out = std::make_shared<const CBlock>(block);

    if (!process_new_block) return true;

    if (!chainman.ProcessNewBlock(block_out, /*force_processing=*/true, /*min_pow_checked=*/true, nullptr)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "ProcessNewBlock, block not accepted");
    }

    return true;
}

static UniValue generateBlocks(ChainstateManager& chainman, const CTxMemPool& mempool, const CScript& coinbase_script, int nGenerate, uint64_t nMaxTries, NodeContext* m_node)
{
    UniValue blockHashes(UniValue::VARR);
    while (nGenerate > 0 && !ShutdownRequested()) {
        std::unique_ptr<CBlockTemplate> pblocktemplate(BlockAssembler(chainman.ActiveChainstate(), &mempool).CreateNewBlock(coinbase_script,nullptr,nullptr,m_node));
        if (!pblocktemplate.get())
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Couldn't create new block");

        std::shared_ptr<const CBlock> block_out;
        if (!GenerateBlock(chainman, pblocktemplate->block, nMaxTries, block_out, /*process_new_block=*/true)) {
            break;
        }

        if (block_out) {
            --nGenerate;
            blockHashes.push_back(block_out->GetHash().GetHex());
        }
    }
    return blockHashes;
}

static bool getScriptFromDescriptor(const std::string& descriptor, CScript& script, std::string& error)
{
    FlatSigningProvider key_provider;
    const auto desc = Parse(descriptor, key_provider, error, /* require_checksum = */ false);
    if (desc) {
        if (desc->IsRange()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Ranged descriptor not accepted. Maybe pass through deriveaddresses first?");
        }

        FlatSigningProvider provider;
        std::vector<CScript> scripts;
        if (!desc->Expand(0, key_provider, scripts, provider)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Cannot derive script without private keys");
        }

        // Combo descriptors can have 2 or 4 scripts, so we can't just check scripts.size() == 1
        CHECK_NONFATAL(scripts.size() > 0 && scripts.size() <= 4);

        if (scripts.size() == 1) {
            script = scripts.at(0);
        } else if (scripts.size() == 4) {
            // For uncompressed keys, take the 3rd script, since it is p2wpkh
            script = scripts.at(2);
        } else {
            // Else take the 2nd script, since it is p2pkh
            script = scripts.at(1);
        }

        return true;
    } else {
        return false;
    }
}

static RPCHelpMan generatetodescriptor()
{
    return RPCHelpMan{
        "generatetodescriptor",
        "Mine to a specified descriptor and return the block hashes.",
        {
            {"num_blocks", RPCArg::Type::NUM, RPCArg::Optional::NO, "How many blocks are generated."},
            {"descriptor", RPCArg::Type::STR, RPCArg::Optional::NO, "The descriptor to send the newly generated bitcoin to."},
            {"maxtries", RPCArg::Type::NUM, RPCArg::Default{DEFAULT_MAX_TRIES}, "How many iterations to try."},
        },
        RPCResult{
            RPCResult::Type::ARR, "", "hashes of blocks generated",
            {
                {RPCResult::Type::STR_HEX, "", "blockhash"},
            }
        },
        RPCExamples{
            "\nGenerate 11 blocks to mydesc\n" + HelpExampleCli("generatetodescriptor", "11 \"mydesc\"")},
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const int num_blocks{request.params[0].getInt<int>()};
    const uint64_t max_tries{request.params[2].isNull() ? DEFAULT_MAX_TRIES : request.params[2].getInt<int>()};

    CScript coinbase_script;
    std::string error;
    if (!getScriptFromDescriptor(request.params[1].get_str(), coinbase_script, error)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, error);
    }

    NodeContext& node = EnsureAnyNodeContext(request.context);
    const CTxMemPool& mempool = EnsureMemPool(node);
    ChainstateManager& chainman = EnsureChainman(node);

    return generateBlocks(chainman, mempool, coinbase_script, num_blocks, max_tries, &node);
},
};
}

static RPCHelpMan generate()
{
    return RPCHelpMan{"generate", "has been replaced by the -generate cli option. Refer to -help for more information.", {}, {}, RPCExamples{""}, [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue {
        throw JSONRPCError(RPC_METHOD_NOT_FOUND, self.ToString());
    }};
}

static RPCHelpMan generatetoaddress()
{
    return RPCHelpMan{"generatetoaddress",
        "Mine to a specified address and return the block hashes.",
         {
             {"nblocks", RPCArg::Type::NUM, RPCArg::Optional::NO, "How many blocks are generated."},
             {"address", RPCArg::Type::STR, RPCArg::Optional::NO, "The address to send the newly generated peercoin to."},
             {"maxtries", RPCArg::Type::NUM, RPCArg::Default{DEFAULT_MAX_TRIES}, "How many iterations to try."},
         },
         RPCResult{
             RPCResult::Type::ARR, "", "hashes of blocks generated",
             {
                 {RPCResult::Type::STR_HEX, "", "blockhash"},
             }},
         RPCExamples{
            "\nGenerate 11 blocks to myaddress\n"
            + HelpExampleCli("generatetoaddress", "11 \"myaddress\"")
            + "If you are using the " PACKAGE_NAME " wallet, you can get a new address to send the newly generated peercoin to with:\n"
            + HelpExampleCli("getnewaddress", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const int num_blocks{request.params[0].getInt<int>()};
    const uint64_t max_tries{request.params[2].isNull() ? DEFAULT_MAX_TRIES : request.params[2].getInt<int>()};

    CTxDestination destination = DecodeDestination(request.params[1].get_str());
    if (!IsValidDestination(destination)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Error: Invalid address");
    }

    NodeContext& node = EnsureAnyNodeContext(request.context);
    const CTxMemPool& mempool = EnsureMemPool(node);
    ChainstateManager& chainman = EnsureChainman(node);

    CScript coinbase_script = GetScriptForDestination(destination);

    return generateBlocks(chainman, mempool, coinbase_script, num_blocks, max_tries, &node);
},
    };
}

static RPCHelpMan generateblock()
{
    return RPCHelpMan{"generateblock",
        "Mine a set of ordered transactions to a specified address or descriptor and return the block hash.",
        {
            {"output", RPCArg::Type::STR, RPCArg::Optional::NO, "The address or descriptor to send the newly generated bitcoin to."},
            {"transactions", RPCArg::Type::ARR, RPCArg::Optional::NO, "An array of hex strings which are either txids or raw transactions.\n"
                "Txids must reference transactions currently in the mempool.\n"
                "All transactions must be valid and in valid order, otherwise the block will be rejected.",
                {
                    {"rawtx/txid", RPCArg::Type::STR_HEX, RPCArg::Optional::OMITTED, ""},
                },
            },
            {"submit", RPCArg::Type::BOOL, RPCArg::Default{true}, "Whether to submit the block before the RPC call returns or to return it as hex."},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR_HEX, "hash", "hash of generated block"},
                {RPCResult::Type::STR_HEX, "hex", /*optional=*/true, "hex of generated block, only present when submit=false"},
            }
        },
        RPCExamples{
            "\nGenerate a block to myaddress, with txs rawtx and mempool_txid\n"
            + HelpExampleCli("generateblock", R"("myaddress" '["rawtx", "mempool_txid"]')")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    const auto address_or_descriptor = request.params[0].get_str();
    CScript coinbase_script;
    std::string error;

    if (!getScriptFromDescriptor(address_or_descriptor, coinbase_script, error)) {
        const auto destination = DecodeDestination(address_or_descriptor);
        if (!IsValidDestination(destination)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Error: Invalid address or descriptor");
        }

        coinbase_script = GetScriptForDestination(destination);
    }

    NodeContext& node = EnsureAnyNodeContext(request.context);
    const CTxMemPool& mempool = EnsureMemPool(node);

    std::vector<CTransactionRef> txs;
    const auto raw_txs_or_txids = request.params[1].get_array();
    for (size_t i = 0; i < raw_txs_or_txids.size(); i++) {
        const auto str(raw_txs_or_txids[i].get_str());

        uint256 hash;
        CMutableTransaction mtx;
        if (ParseHashStr(str, hash)) {

            const auto tx = mempool.get(hash);
            if (!tx) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, strprintf("Transaction %s not in mempool.", str));
            }

            txs.emplace_back(tx);

        } else if (DecodeHexTx(mtx, str)) {
            txs.push_back(MakeTransactionRef(std::move(mtx)));

        } else {
            throw JSONRPCError(RPC_DESERIALIZATION_ERROR, strprintf("Transaction decode failed for %s. Make sure the tx has at least one input.", str));
        }
    }

    const bool process_new_block{request.params[2].isNull() ? true : request.params[2].get_bool()};
    CBlock block;

    ChainstateManager& chainman = EnsureChainman(node);
    {
        LOCK(cs_main);

        std::unique_ptr<CBlockTemplate> blocktemplate(BlockAssembler{chainman.ActiveChainstate(), nullptr}.CreateNewBlock(coinbase_script));
        if (!blocktemplate) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Couldn't create new block");
        }
        block = blocktemplate->block;
    }

    CHECK_NONFATAL(block.vtx.size() == 1);

    // Add transactions
    block.vtx.insert(block.vtx.end(), txs.begin(), txs.end());
    RegenerateCommitments(block, chainman);

    {
        LOCK(cs_main);

        BlockValidationState state;
        if (!TestBlockValidity(state, chainman.GetParams(), chainman.ActiveChainstate(), block, chainman.m_blockman.LookupBlockIndex(block.hashPrevBlock), GetAdjustedTime, false, false)) {
            throw JSONRPCError(RPC_VERIFY_ERROR, strprintf("TestBlockValidity failed: %s", state.ToString()));
        }
    }

    std::shared_ptr<const CBlock> block_out;
    uint64_t max_tries{DEFAULT_MAX_TRIES};

    if (!GenerateBlock(chainman, block, max_tries, block_out, process_new_block) || !block_out) {
        throw JSONRPCError(RPC_MISC_ERROR, "Failed to make block.");
    }

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("hash", block_out->GetHash().GetHex());
    if (!process_new_block) {
        CDataStream block_ser{SER_NETWORK, PROTOCOL_VERSION | RPCSerializationFlags()};
        block_ser << *block_out;
        obj.pushKV("hex", HexStr(block_ser));
    }
    return obj;
},
    };
}

static RPCHelpMan getmininginfo()
{
    return RPCHelpMan{"getmininginfo",
                "\nReturns a json object containing validation-related information.\n"
                "Note: BT2C is a PoS-only blockchain, so mining-related fields return 0.\n",
                {},
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::NUM, "blocks", "The current block"},
                        {RPCResult::Type::NUM, "currentblockweight", /*optional=*/true, "The block weight of the last assembled block (only present if a block was ever assembled)"},
                        {RPCResult::Type::NUM, "currentblocktx", /*optional=*/true, "The number of block transactions of the last assembled block (only present if a block was ever assembled)"},
                        {RPCResult::Type::NUM, "difficulty", "The current difficulty"},
                        {RPCResult::Type::NUM, "networkhashps", "The network hashes per second (always 0 in BT2C)"},
                        {RPCResult::Type::NUM, "networkghps", "The network gigahashes per second (always 0 in BT2C)"},
                        {RPCResult::Type::NUM, "pooledtx", "The size of the mempool"},
                        {RPCResult::Type::STR, "chain", "current network name (main, test, signet, regtest)"},
                        {RPCResult::Type::STR, "warnings", "any network and blockchain warnings"},
                    }},
                RPCExamples{
                    HelpExampleCli("getmininginfo", "")
            + HelpExampleRpc("getmininginfo", "")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
    const CTxMemPool& mempool = EnsureMemPool(node);
    ChainstateManager& chainman = EnsureChainman(node);
    LOCK(cs_main);
    const CChain& active_chain = chainman.ActiveChain();

    UniValue obj(UniValue::VOBJ);
    obj.pushKV("blocks",           active_chain.Height());
    if (BlockAssembler::m_last_block_weight) obj.pushKV("currentblockweight", *BlockAssembler::m_last_block_weight);
    if (BlockAssembler::m_last_block_num_txs) obj.pushKV("currentblocktx", *BlockAssembler::m_last_block_num_txs);
    obj.pushKV("difficulty",       (double)GetDifficulty(active_chain.Tip(), active_chain.Tip()));
    obj.pushKV("networkhashps",    getnetworkhashps().HandleRequest(request));
    obj.pushKV("networkghps",      getnetworkghps().HandleRequest(request));
    obj.pushKV("pooledtx",         (uint64_t)mempool.size());
    obj.pushKV("chain", chainman.GetParams().NetworkIDString());
    obj.pushKV("warnings",         GetWarnings(false).original);
    return obj;
},
    };
}



// NOTE: Assumes a conclusive result; if result is inconclusive, it must be handled by caller
static UniValue BIP22ValidationResult(const BlockValidationState& state)
{
    if (state.IsValid())
        return UniValue::VNULL;

    if (state.IsError())
        throw JSONRPCError(RPC_VERIFY_ERROR, state.ToString());
    if (state.IsInvalid())
    {
        std::string strRejectReason = state.GetRejectReason();
        if (strRejectReason.empty())
            return "rejected";
        return strRejectReason;
    }
    // Should be impossible
    return "valid?";
}

static RPCHelpMan getblocktemplate()
{
    return RPCHelpMan{"getblocktemplate",
        "\nIf the request parameters include a 'mode' key, that is used to explicitly select between the default 'template' request or a 'proposal'.\n"
        "It returns data needed to construct a block to work on.\n"
        "For full specification, see BIPs 22, 23, 9, and 145:\n"
        "    https://github.com/bitcoin/bips/blob/master/bip-0022.mediawiki\n"
        "    https://github.com/bitcoin/bips/blob/master/bip-0023.mediawiki\n"
        "    https://github.com/bitcoin/bips/blob/master/bip-0009.mediawiki#getblocktemplate_changes\n"
        "    https://github.com/bitcoin/bips/blob/master/bip-0145.mediawiki\n",
        {
            {"template_request", RPCArg::Type::OBJ, RPCArg::Default{UniValue::VOBJ}, "Format of the template",
            {
                {"mode", RPCArg::Type::STR, /* treat as named arg */ RPCArg::Optional::OMITTED, "This must be set to \"template\", \"proposal\" (see BIP 23), or omitted"},
                {"capabilities", RPCArg::Type::ARR, /* treat as named arg */ RPCArg::Optional::OMITTED, "A list of strings",
                {
                    {"str", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "client side supported feature, 'longpoll', 'coinbasevalue', 'proposal', 'serverlist', 'workid'"},
                }},
                {"rules", RPCArg::Type::ARR, RPCArg::Optional::NO, "A list of strings",
                {
                    {"segwit", RPCArg::Type::STR, RPCArg::Optional::NO, "(literal) indicates client side segwit support"},
                    {"str", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "other client side supported softfork deployment"},
                }},
            },
            RPCArgOptions{.oneline_description="\"template_request\""}},
        },
        {
            RPCResult{"If the proposal was accepted with mode=='proposal'", RPCResult::Type::NONE, "", ""},
            RPCResult{"If the proposal was not accepted with mode=='proposal'", RPCResult::Type::STR, "", "According to BIP22"},
            RPCResult{"Otherwise", RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::NUM, "version", "The preferred block version"},
                {RPCResult::Type::ARR, "rules", "specific block rules that are to be enforced",
                {
                    {RPCResult::Type::STR, "", "name of a rule the client must understand to some extent; see BIP 9 for format"},
                }},
                {RPCResult::Type::ARR, "capabilities", "",
                {
                    {RPCResult::Type::STR, "value", "A supported feature, for example 'proposal'"},
                }},
                {RPCResult::Type::STR, "previousblockhash", "The hash of current highest block"},
                {RPCResult::Type::ARR, "transactions", "contents of non-coinbase transactions that should be included in the next block",
                {
                    {RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::STR_HEX, "data", "transaction data encoded in hexadecimal (byte-for-byte)"},
                        {RPCResult::Type::STR_HEX, "txid", "transaction id encoded in little-endian hexadecimal"},
                        {RPCResult::Type::STR_HEX, "hash", "hash encoded in little-endian hexadecimal (including witness data)"},
                        {RPCResult::Type::ARR, "depends", "array of numbers",
                        {
                            {RPCResult::Type::NUM, "", "transactions before this one (by 1-based index in 'transactions' list) that must be present in the final block if this one is"},
                        }},
                        {RPCResult::Type::NUM, "fee", "difference in value between transaction inputs and outputs (in satoshis); for coinbase transactions, this is a negative Number of the total collected block fees (ie, not including the block subsidy); if key is not present, fee is unknown and clients MUST NOT assume there isn't one"},
                        {RPCResult::Type::NUM, "sigops", "total SigOps cost, as counted for purposes of block limits; if key is not present, sigop cost is unknown and clients MUST NOT assume it is zero"},
                        {RPCResult::Type::NUM, "weight", "total transaction weight, as counted for purposes of block limits"},
                    }},
                }},
                {RPCResult::Type::OBJ_DYN, "coinbaseaux", "data that should be included in the coinbase's scriptSig content",
                {
                    {RPCResult::Type::STR_HEX, "key", "values must be in the coinbase (keys may be ignored)"},
                }},
                {RPCResult::Type::NUM, "coinbasevalue", "maximum allowable input to coinbase transaction, including the generation award and transaction fees (in satoshis)"},
                {RPCResult::Type::STR, "longpollid", "an id to include with a request to longpoll on an update to this template"},
                {RPCResult::Type::STR, "target", "The hash target"},
                {RPCResult::Type::NUM_TIME, "mintime", "The minimum timestamp appropriate for the next block time, expressed in " + UNIX_EPOCH_TIME},
                {RPCResult::Type::ARR, "mutable", "list of ways the block template may be changed",
                {
                    {RPCResult::Type::STR, "value", "A way the block template may be changed, e.g. 'time', 'transactions', 'prevblock'"},
                }},
                {RPCResult::Type::STR_HEX, "noncerange", "A range of valid nonces"},
                {RPCResult::Type::NUM, "sigoplimit", "limit of sigops in blocks"},
                {RPCResult::Type::NUM, "sizelimit", "limit of block size"},
                {RPCResult::Type::NUM, "weightlimit", /*optional=*/true, "limit of block weight"},
                {RPCResult::Type::NUM_TIME, "curtime", "current timestamp in " + UNIX_EPOCH_TIME},
                {RPCResult::Type::STR, "bits", "compressed target of next block"},
                {RPCResult::Type::NUM, "height", "The height of the next block"},
                {RPCResult::Type::STR_HEX, "signet_challenge", /*optional=*/true, "Only on signet"},
                {RPCResult::Type::STR_HEX, "default_witness_commitment", /*optional=*/true, "a valid witness commitment for the unmodified block template"},
            }},
        },
        RPCExamples{
                    HelpExampleCli("getblocktemplate", "'{\"rules\": [\"segwit\"]}'")
            + HelpExampleRpc("getblocktemplate", "{\"rules\": [\"segwit\"]}")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    NodeContext& node = EnsureAnyNodeContext(request.context);
    ChainstateManager& chainman = EnsureChainman(node);
    LOCK(cs_main);

    std::string strMode = "template";
    UniValue lpval = NullUniValue;
    std::set<std::string> setClientRules;
    Chainstate& active_chainstate = chainman.ActiveChainstate();
    CChain& active_chain = active_chainstate.m_chain;
    if (!request.params[0].isNull())
    {
        const UniValue& oparam = request.params[0].get_obj();
        const UniValue& modeval = find_value(oparam, "mode");
        if (modeval.isStr())
            strMode = modeval.get_str();
        else if (modeval.isNull())
        {
            /* Do nothing */
        }
        else
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");
        lpval = find_value(oparam, "longpollid");

        if (strMode == "proposal")
        {
            const UniValue& dataval = find_value(oparam, "data");
            if (!dataval.isStr())
                throw JSONRPCError(RPC_TYPE_ERROR, "Missing data String key for proposal");

            CBlock block;
            if (!DecodeHexBlk(block, dataval.get_str()))
                throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block decode failed");

            uint256 hash = block.GetHash();
            const CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(hash);
            if (pindex) {
                if (pindex->IsValid(BLOCK_VALID_SCRIPTS))
                    return "duplicate";
                if (pindex->nStatus & BLOCK_FAILED_MASK)
                    return "duplicate-invalid";
                return "duplicate-inconclusive";
            }

            CBlockIndex* const pindexPrev = active_chain.Tip();
            // TestBlockValidity only supports blocks built on the current Tip
            if (block.hashPrevBlock != pindexPrev->GetBlockHash())
                return "inconclusive-not-best-prevblk";
            BlockValidationState state;
            TestBlockValidity(state, chainman.GetParams(), active_chainstate, block, pindexPrev, GetAdjustedTime, false, true);
            return BIP22ValidationResult(state);
        }

        const UniValue& aClientRules = find_value(oparam, "rules");
        if (aClientRules.isArray()) {
            for (unsigned int i = 0; i < aClientRules.size(); ++i) {
                const UniValue& v = aClientRules[i];
                setClientRules.insert(v.get_str());
            }
        }
    }

    if (strMode != "template")
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid mode");

    if (!chainman.GetParams().IsTestChain()) {
        const CConnman& connman = EnsureConnman(node);
        if (connman.GetNodeCount(ConnectionDirection::Both) == 0) {
            throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, PACKAGE_NAME " is not connected!");
        }

        if (active_chainstate.IsInitialBlockDownload()) {
            throw JSONRPCError(RPC_CLIENT_IN_INITIAL_DOWNLOAD, PACKAGE_NAME " is in initial sync and waiting for blocks...");
        }
    }

    static unsigned int nTransactionsUpdatedLast;
    const CTxMemPool& mempool = EnsureMemPool(node);

    if (!lpval.isNull())
    {
        // Wait to respond until either the best block changes, OR a minute has passed and there are more transactions
        uint256 hashWatchedChain;
        std::chrono::steady_clock::time_point checktxtime;
        unsigned int nTransactionsUpdatedLastLP;

        if (lpval.isStr())
        {
            // Format: <hashBestChain><nTransactionsUpdatedLast>
            const std::string& lpstr = lpval.get_str();

            hashWatchedChain = ParseHashV(lpstr.substr(0, 64), "longpollid");
            nTransactionsUpdatedLastLP = LocaleIndependentAtoi<int64_t>(lpstr.substr(64));
        }
        else
        {
            // NOTE: Spec does not specify behaviour for non-string longpollid, but this makes testing easier
            hashWatchedChain = active_chain.Tip()->GetBlockHash();
            nTransactionsUpdatedLastLP = nTransactionsUpdatedLast;
        }

        // Release lock while waiting
        LEAVE_CRITICAL_SECTION(cs_main);
        {
            checktxtime = std::chrono::steady_clock::now() + std::chrono::minutes(1);

            WAIT_LOCK(g_best_block_mutex, lock);
            while (g_best_block == hashWatchedChain && IsRPCRunning())
            {
                if (g_best_block_cv.wait_until(lock, checktxtime) == std::cv_status::timeout)
                {
                    // Timeout: Check transactions for update
                    // without holding the mempool lock to avoid deadlocks
                    if (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLastLP)
                        break;
                    checktxtime += std::chrono::seconds(10);
                }
            }
        }
        ENTER_CRITICAL_SECTION(cs_main);

        if (!IsRPCRunning())
            throw JSONRPCError(RPC_CLIENT_NOT_CONNECTED, "Shutting down");
        // TODO: Maybe recheck connections/IBD and (if something wrong) send an expires-immediately template to stop miners?
    }

    const Consensus::Params& consensusParams = chainman.GetParams().GetConsensus();

    // GBT must be called with 'signet' set in the rules for signet chains
    if (consensusParams.signet_blocks && setClientRules.count("signet") != 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "getblocktemplate must be called with the signet rule set (call with {\"rules\": [\"segwit\", \"signet\"]})");
    }

    // GBT must be called with 'segwit' set in the rules
    if (setClientRules.count("segwit") != 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "getblocktemplate must be called with the segwit rule set (call with {\"rules\": [\"segwit\"]})");
    }

    // Update block
    static CBlockIndex* pindexPrev;
    static int64_t time_start;
    static std::unique_ptr<CBlockTemplate> pblocktemplate;
    if (pindexPrev != active_chain.Tip() ||
        (mempool.GetTransactionsUpdated() != nTransactionsUpdatedLast && GetTime() - time_start > 5))
    {
        // Clear pindexPrev so future calls make a new block, despite any failures from here on
        pindexPrev = nullptr;

        // Store the pindexBest used before CreateNewBlock, to avoid races
        nTransactionsUpdatedLast = mempool.GetTransactionsUpdated();
        CBlockIndex* pindexPrevNew = active_chain.Tip();
        time_start = GetTime();

        // Create new block
        CScript scriptDummy = CScript() << OP_TRUE;
        pblocktemplate = BlockAssembler(active_chainstate, &mempool).CreateNewBlock(scriptDummy, nullptr, nullptr, &node);
        if (!pblocktemplate)
            throw JSONRPCError(RPC_OUT_OF_MEMORY, "Out of memory");

        // Need to update only after we know CreateNewBlock succeeded
        pindexPrev = pindexPrevNew;
    }
    CHECK_NONFATAL(pindexPrev);
    CBlock* pblock = &pblocktemplate->block; // pointer for convenience

    // Update nTime
    UpdateTime(pblock, consensusParams, pindexPrev);
    pblock->nNonce = 0;

    // NOTE: If at some point we support pre-segwit miners post-segwit-activation, this needs to take segwit support into consideration
    const bool fPreSegWit = !IsBTC16BIPsEnabled(active_chain.Tip()->nTime);

    UniValue aCaps(UniValue::VARR); aCaps.push_back("proposal");

    UniValue transactions(UniValue::VARR);
    std::map<uint256, int64_t> setTxIndex;
    int i = 0;
    for (const auto& it : pblock->vtx) {
        const CTransaction& tx = *it;
        uint256 txHash = tx.GetHash();
        setTxIndex[txHash] = i++;

        if (tx.IsCoinBase())
            continue;

        UniValue entry(UniValue::VOBJ);

        entry.pushKV("data", EncodeHexTx(tx));
        entry.pushKV("txid", txHash.GetHex());
        entry.pushKV("hash", tx.GetWitnessHash().GetHex());

        UniValue deps(UniValue::VARR);
        for (const CTxIn &in : tx.vin)
        {
            if (setTxIndex.count(in.prevout.hash))
                deps.push_back(setTxIndex[in.prevout.hash]);
        }
        entry.pushKV("depends", deps);

        int index_in_template = i - 1;
        entry.pushKV("fee", pblocktemplate->vTxFees[index_in_template]);
        int64_t nTxSigOps = pblocktemplate->vTxSigOpsCost[index_in_template];
        if (fPreSegWit) {
            CHECK_NONFATAL(nTxSigOps % WITNESS_SCALE_FACTOR == 0);
            nTxSigOps /= WITNESS_SCALE_FACTOR;
        }
        entry.pushKV("sigops", nTxSigOps);
        entry.pushKV("weight", GetTransactionWeight(tx));

        transactions.push_back(entry);
    }

    UniValue aux(UniValue::VOBJ);

    arith_uint256 hashTarget = arith_uint256().SetCompact(pblock->nBits);

    UniValue aMutable(UniValue::VARR);
    aMutable.push_back("time");
    aMutable.push_back("transactions");
    aMutable.push_back("prevblock");

    UniValue result(UniValue::VOBJ);
    result.pushKV("capabilities", aCaps);

    UniValue aRules(UniValue::VARR);
    aRules.push_back("csv");
    if (!fPreSegWit) aRules.push_back("!segwit");
    if (consensusParams.signet_blocks) {
        // indicate to miner that they must understand signet rules
        // when attempting to mine with this template
        aRules.push_back("!signet");
    }

    result.pushKV("version", pblock->nVersion);
    result.pushKV("rules", aRules);

/*
    if (nMaxVersionPreVB >= 2) {
        // Because BIP 34 changed how the generation transaction is serialized, we can only use version/force back to v2 blocks
        // This is safe to do [otherwise-]unconditionally only because we are throwing an exception above if a non-force deployment gets activated
        aMutable.push_back("version/force");
    }
*/

    result.pushKV("previousblockhash", pblock->hashPrevBlock.GetHex());
    result.pushKV("transactions", transactions);
    result.pushKV("coinbaseaux", aux);
    result.pushKV("coinbasevalue", (int64_t)pblock->vtx[0]->vout[0].nValue);
    result.pushKV("longpollid", active_chain.Tip()->GetBlockHash().GetHex() + ToString(nTransactionsUpdatedLast));
    result.pushKV("target", hashTarget.GetHex());
    result.pushKV("mintime", (int64_t)pindexPrev->GetMedianTimePast()+1);
    result.pushKV("mutable", aMutable);
    result.pushKV("noncerange", "00000000ffffffff");
    int64_t nSigOpLimit = MAX_BLOCK_SIGOPS_COST;
    int64_t nSizeLimit = MAX_BLOCK_SERIALIZED_SIZE;
    if (fPreSegWit) {
        CHECK_NONFATAL(nSigOpLimit % WITNESS_SCALE_FACTOR == 0);
        nSigOpLimit /= WITNESS_SCALE_FACTOR;
        CHECK_NONFATAL(nSizeLimit % WITNESS_SCALE_FACTOR == 0);
        nSizeLimit /= WITNESS_SCALE_FACTOR;
    }
    result.pushKV("sigoplimit", nSigOpLimit);
    result.pushKV("sizelimit", nSizeLimit);
    if (!fPreSegWit) {
        result.pushKV("weightlimit", (int64_t)MAX_BLOCK_WEIGHT);
    }
    result.pushKV("curtime", pblock->GetBlockTime());
    result.pushKV("bits", strprintf("%08x", pblock->nBits));
    result.pushKV("height", (int64_t)(pindexPrev->nHeight+1));

    if (consensusParams.signet_blocks) {
        result.pushKV("signet_challenge", HexStr(consensusParams.signet_challenge));
    }

    if (!pblocktemplate->vchCoinbaseCommitment.empty()) {
        result.pushKV("default_witness_commitment", HexStr(pblocktemplate->vchCoinbaseCommitment));
    }

    return result;
},
    };
}

class submitblock_StateCatcher final : public CValidationInterface
{
public:
    uint256 hash;
    bool found{false};
    BlockValidationState state;

    explicit submitblock_StateCatcher(const uint256 &hashIn) : hash(hashIn), state() {}

protected:
    void BlockChecked(const CBlock& block, const BlockValidationState& stateIn) override {
        if (block.GetHash() != hash)
            return;
        found = true;
        state = stateIn;
    }
};

static RPCHelpMan submitblock()
{
    // We allow 2 arguments for compliance with BIP22. Argument 2 is ignored.
    return RPCHelpMan{"submitblock",
        "\nAttempts to submit new block to network.\n"
        "See https://en.bitcoin.it/wiki/BIP_0022 for full specification.\n",
        {
            {"hexdata", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "the hex-encoded block data to submit"},
            {"dummy", RPCArg::Type::STR, RPCArg::DefaultHint{"ignored"}, "dummy value, for compatibility with BIP22. This value is ignored."},
        },
        {
            RPCResult{"If the block was accepted", RPCResult::Type::NONE, "", ""},
            RPCResult{"Otherwise", RPCResult::Type::STR, "", "According to BIP22"},
        },
        RPCExamples{
                    HelpExampleCli("submitblock", "\"mydata\"")
            + HelpExampleRpc("submitblock", "\"mydata\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    std::shared_ptr<CBlock> blockptr = std::make_shared<CBlock>();
    CBlock& block = *blockptr;
    if (!DecodeHexBlk(block, request.params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block decode failed");
    }

    if (block.vtx.empty() || !block.vtx[0]->IsCoinBase()) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block does not start with a coinbase");
    }

    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    uint256 hash = block.GetHash();
    {
        LOCK(cs_main);
        const CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(hash);
        if (pindex) {
            if (pindex->IsValid(BLOCK_VALID_SCRIPTS)) {
                return "duplicate";
            }
            if (pindex->nStatus & BLOCK_FAILED_MASK) {
                return "duplicate-invalid";
            }
        }
    }

    // peercoin: check block before attempting to sign it
    BlockValidationState state;
    if (!CheckBlock(block, state, Params().GetConsensus(), true,  true, false)) {
        LogPrintf("SubmitBlock: %s\n", state.ToString());
        throw JSONRPCError(-100, "Block failed CheckBlock() function.");
        }

    {
        LOCK(cs_main);
        const CBlockIndex* pindex = chainman.m_blockman.LookupBlockIndex(block.hashPrevBlock);
        if (pindex) {
            chainman.UpdateUncommittedBlockStructures(block, pindex);
        }
    }

    bool new_block;
    auto sc = std::make_shared<submitblock_StateCatcher>(block.GetHash());
    RegisterSharedValidationInterface(sc);
    bool accepted = chainman.ProcessNewBlock(blockptr, /*force_processing=*/true, /*min_pow_checked=*/true, /*new_block=*/&new_block);
    UnregisterSharedValidationInterface(sc);
    if (!new_block && accepted) {
        return "duplicate";
    }
    if (!sc->found) {
        return "inconclusive";
    }
    return BIP22ValidationResult(sc->state);
},
    };
}

static RPCHelpMan submitheader()
{
    return RPCHelpMan{"submitheader",
                "\nDecode the given hexdata as a header and submit it as a candidate chain tip if valid."
                "\nThrows when the header is invalid.\n"
                "\nNote: BT2C is a PoS-only blockchain, so PoW headers will be rejected.\n",
                {
                    {"hexdata", RPCArg::Type::STR_HEX, RPCArg::Optional::NO, "the hex-encoded block header data"},
                },
                RPCResult{
                    RPCResult::Type::NONE, "", "None"},
                RPCExamples{
                    HelpExampleCli("submitheader", "\"aabbcc\"") +
                    HelpExampleRpc("submitheader", "\"aabbcc\"")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    CBlockHeader h;
    if (!DecodeHexBlockHeader(h, request.params[0].get_str())) {
        throw JSONRPCError(RPC_DESERIALIZATION_ERROR, "Block header decode failed");
    }
    
    // BT2C: For BT2C, all blocks must be PoS, so reject any potential PoW blocks
    // Since we can't call IsProofOfWork() on CBlockHeader directly, we'll assume it's PoW
    // if it doesn't have the PoS flag set (nFlags & CBlockIndex::BLOCK_PROOF_OF_STAKE)
    if (!(h.nFlags & CBlockIndex::BLOCK_PROOF_OF_STAKE)) {
        throw JSONRPCError(RPC_VERIFY_ERROR, "BT2C is a PoS-only blockchain, PoW headers are not accepted");
    }
    
    ChainstateManager& chainman = EnsureAnyChainman(request.context);
    {
        LOCK(cs_main);
        if (!chainman.m_blockman.LookupBlockIndex(h.hashPrevBlock)) {
            throw JSONRPCError(RPC_VERIFY_ERROR, "Must submit previous header (" + h.hashPrevBlock.GetHex() + ") first");
        }
    }

    BlockValidationState state;
    int tmpTemp;
    CBlockIndex* tip;
    {
        LOCK(cs_main);
        tip = chainman.ActiveChain().Tip();
    }
    chainman.ProcessNewBlockHeaders(tmpTemp, tip->GetBlockHash(), {h}, /*min_pow_checked=*/true, state, Params());
    if (state.IsValid()) return NullUniValue;
    if (state.IsError()) {
        throw JSONRPCError(RPC_VERIFY_ERROR, state.ToString());
    }
    throw JSONRPCError(RPC_VERIFY_ERROR, state.GetRejectReason());
},
    };
}
/*
static RPCHelpMan estimatesmartfee()
{
    return RPCHelpMan{"estimatesmartfee",
        "\nEstimates the approximate fee per kilobyte needed for a transaction to begin\n"
        "confirmation within conf_target blocks if possible and return the number of blocks\n"
        "for which the estimate is valid. Uses virtual transaction size as defined\n"
        "in BIP 141 (witness data is discounted).\n",
        {
            {"conf_target", RPCArg::Type::NUM, RPCArg::Optional::NO, "Confirmation target in blocks (1 - 1008)"},
            {"estimate_mode", RPCArg::Type::STR, RPCArg::Default{"conservative"}, "The fee estimate mode.\n"
            "                   Whether to return a more conservative estimate which also satisfies\n"
            "                   a longer history. A conservative estimate potentially returns a\n"
            "                   higher feerate and is more likely to be sufficient for the desired\n"
            "                   target, but is not as responsive to short term drops in the\n"
            "                   prevailing fee market. Must be one of (case insensitive):\n"
             "\""},
                },
                RPCResult{
                    RPCResult::Type::OBJ, "", "",
                    {
                        {RPCResult::Type::NUM, "feerate", true, "estimate fee rate in " + CURRENCY_UNIT + "/kvB (only present if no errors were encountered)"},
                        {RPCResult::Type::ARR, "errors", true, "Errors encountered during processing (if there are any)",
                            {
                                {RPCResult::Type::STR, "", "error"},
                            }},
                        {RPCResult::Type::NUM, "blocks", "block number where estimate was found\n"
            "The request target will be clamped between 2 and the highest target\n"
            "fee estimation is able to return based on how long it has been running.\n"
            "An error is returned if not enough transactions and blocks\n"
            "have been observed to make an estimate for any number of blocks."},
                    }},
                RPCExamples{
                    HelpExampleCli("estimatesmartfee", "6") +
                    HelpExampleRpc("estimatesmartfee", "6")
                },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
{
    RPCTypeCheck(request.params, {UniValue::VNUM, UniValue::VSTR});
    RPCTypeCheckArgument(request.params[0], UniValue::VNUM);

    ChainstateManager& chainman = EnsureAnyChainman(request.context);

    UniValue result(UniValue::VOBJ);
    result.pushKV("feerate", 0.01);
    result.pushKV("blocks", chainman.ActiveChain().Height());
    return result;
},
};
}
*/
void RegisterMiningRPCCommands(CRPCTable& t)
{
    static const CRPCCommand commands[]{
        {"mining", &getnetworkhashps},
        {"mining", &getmininginfo},
        {"mining", &getblocktemplate},
        {"mining", &submitblock},
        {"mining", &submitheader},

        {"hidden", &generatetoaddress},
        {"hidden", &generatetodescriptor},
        {"hidden", &generateblock},
        {"hidden", &generate},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
