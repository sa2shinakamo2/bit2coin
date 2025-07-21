// Copyright (c) 2025 BT2C Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <rpc/server.h>
#include <rpc/util.h>
#include <validation.h>
#include <validator.h>
#include <wallet/wallet.h>
#include <wallet/rpc/util.h>
#include <wallet/spend.h>
#include <wallet/receive.h>
#include <key_io.h>
#include <script/standard.h>
#include <consensus/amount.h>
#include <util/strencodings.h>
#include <util/moneystr.h>
#include <univalue.h>

using namespace wallet;

// Global validator registry
extern CValidatorRegistry g_validatorRegistry;

static RPCHelpMan registervalidator()
{
    return RPCHelpMan{"registervalidator",
        "\nRegister as a validator with the specified stake amount.\n"
        "Requires minimum 32 BTC stake to participate in block production.\n",
        {
            {"amount", RPCArg::Type::AMOUNT, RPCArg::Optional::NO, "The amount to stake (minimum 32 BTC)"},
            {"address", RPCArg::Type::STR, RPCArg::Optional::OMITTED, "The address to use for validator registration (default: new address)"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "validator_id", "The validator ID (hash)"},
                {RPCResult::Type::STR, "address", "The validator address"},
                {RPCResult::Type::NUM, "stake_amount", "The staked amount in BTC"},
                {RPCResult::Type::STR, "status", "Registration status"},
                {RPCResult::Type::NUM, "registration_time", "Registration timestamp"},
            }
        },
        RPCExamples{
            HelpExampleCli("registervalidator", "32.0")
            + HelpExampleCli("registervalidator", "50.0 \"PNYUTqmBT8rgUBgdMwxWYumLznS5CL1hCi\"")
            + HelpExampleRpc("registervalidator", "32.0")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::shared_ptr<CWallet> const pwallet = GetWalletForJSONRPCRequest(request);
            if (!pwallet) return NullUniValue;

            LOCK(pwallet->cs_wallet);

            // Parse stake amount
            CAmount stake_amount = AmountFromValue(request.params[0]);
            
            // Check minimum stake requirement (32 BTC)
            if (stake_amount < 32 * COIN) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Minimum validator stake is 32 BTC");
            }

            // Check wallet balance
            CAmount available_balance = GetBalance(*pwallet).m_mine_trusted;
            if (available_balance < stake_amount) {
                throw JSONRPCError(RPC_WALLET_INSUFFICIENT_FUNDS, 
                    strprintf("Insufficient funds. Available: %s, Required: %s", 
                             FormatMoney(available_balance), FormatMoney(stake_amount)));
            }

            // Get or create address for validator
            CTxDestination dest;
            if (request.params.size() > 1 && !request.params[1].isNull()) {
                // Use provided address
                dest = DecodeDestination(request.params[1].get_str());
                if (!IsValidDestination(dest)) {
                    throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
                }
            } else {
                // Generate new address
                auto op_dest = pwallet->GetNewDestination(OutputType::LEGACY, "validator");
                if (!op_dest) {
                    throw JSONRPCError(RPC_WALLET_KEYPOOL_RAN_OUT, "Failed to get new address");
                }
                dest = *op_dest;
            }

            // Create script for validator
            CScript scriptPubKey = GetScriptForDestination(dest);
            
            // Register validator
            int64_t registration_time = GetTime();
            bool success = g_validatorRegistry.RegisterValidator(scriptPubKey, stake_amount, registration_time);
            
            if (!success) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to register validator");
            }

            // Calculate validator ID
            uint256 validator_id = Hash(scriptPubKey);

            // Return result
            UniValue result(UniValue::VOBJ);
            result.pushKV("validator_id", validator_id.ToString());
            result.pushKV("address", EncodeDestination(dest));
            result.pushKV("stake_amount", FormatMoney(stake_amount));
            result.pushKV("status", "registered");
            result.pushKV("registration_time", registration_time);

            return result;
        },
    };
}

static RPCHelpMan removevalidator()
{
    return RPCHelpMan{"removevalidator",
        "\nRemove validator registration and unstake coins.\n",
        {
            {"validator_id", RPCArg::Type::STR, RPCArg::Optional::NO, "The validator ID to remove"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "validator_id", "The removed validator ID"},
                {RPCResult::Type::STR, "status", "Removal status"},
                {RPCResult::Type::NUM, "unstaked_amount", "The amount unstaked"},
            }
        },
        RPCExamples{
            HelpExampleCli("removevalidator", "\"0123456789abcdef...\"")
            + HelpExampleRpc("removevalidator", "\"0123456789abcdef...\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            // Parse validator ID
            uint256 validator_id;
            validator_id.SetHex(request.params[0].get_str());

            // Get validator before removal
            CValidator* validator = g_validatorRegistry.GetValidator(validator_id);
            if (!validator) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Validator not found");
            }

            CAmount unstaked_amount = validator->stakedAmount;

            // Remove validator
            bool success = g_validatorRegistry.RemoveValidator(validator_id);
            
            if (!success) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to remove validator");
            }

            // Return result
            UniValue result(UniValue::VOBJ);
            result.pushKV("validator_id", validator_id.ToString());
            result.pushKV("status", "removed");
            result.pushKV("unstaked_amount", FormatMoney(unstaked_amount));

            return result;
        },
    };
}

static RPCHelpMan listvalidators()
{
    return RPCHelpMan{"listvalidators",
        "\nList all registered validators.\n",
        {},
        RPCResult{
            RPCResult::Type::ARR, "", "",
            {
                {RPCResult::Type::OBJ, "", "",
                {
                    {RPCResult::Type::STR, "validator_id", "The validator ID"},
                    {RPCResult::Type::STR, "address", "The validator address"},
                    {RPCResult::Type::NUM, "stake_amount", "The staked amount"},
                    {RPCResult::Type::STR, "status", "Validator status (active/slashed)"},
                    {RPCResult::Type::NUM, "reputation", "Validator reputation score"},
                    {RPCResult::Type::NUM, "registration_time", "Registration timestamp"},
                }},
            }
        },
        RPCExamples{
            HelpExampleCli("listvalidators", "")
            + HelpExampleRpc("listvalidators", "")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            std::vector<CValidator*> validators = g_validatorRegistry.GetActiveValidators();
            
            UniValue result(UniValue::VARR);
            
            for (CValidator* validator : validators) {
                if (!validator) continue;
                
                // Calculate validator ID
                uint256 validator_id = Hash(validator->scriptPubKey);
                
                // Extract destination from script
                CTxDestination dest;
                if (ExtractDestination(validator->scriptPubKey, dest)) {
                    UniValue entry(UniValue::VOBJ);
                    entry.pushKV("validator_id", validator_id.ToString());
                    entry.pushKV("address", EncodeDestination(dest));
                    entry.pushKV("stake_amount", FormatMoney(validator->stakedAmount));
                    entry.pushKV("status", validator->status == ValidatorStatus::ACTIVE ? "active" : "slashed");
                    entry.pushKV("reputation", static_cast<int>(validator->reputation.reputationScore));
                    entry.pushKV("registration_time", validator->registrationTime);
                    
                    result.push_back(entry);
                }
            }
            
            return result;
        },
    };
}

static RPCHelpMan getvalidatorinfo()
{
    return RPCHelpMan{"getvalidatorinfo",
        "\nGet information about a specific validator.\n",
        {
            {"validator_id", RPCArg::Type::STR, RPCArg::Optional::NO, "The validator ID"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::STR, "validator_id", "The validator ID"},
                {RPCResult::Type::STR, "address", "The validator address"},
                {RPCResult::Type::NUM, "stake_amount", "The staked amount"},
                {RPCResult::Type::STR, "status", "Validator status"},
                {RPCResult::Type::NUM, "reputation", "Validator reputation score"},
                {RPCResult::Type::NUM, "registration_time", "Registration timestamp"},
                {RPCResult::Type::BOOL, "meets_minimum_stake", "Whether validator meets minimum stake"},
            }
        },
        RPCExamples{
            HelpExampleCli("getvalidatorinfo", "\"0123456789abcdef...\"")
            + HelpExampleRpc("getvalidatorinfo", "\"0123456789abcdef...\"")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            // Parse validator ID
            uint256 validator_id;
            validator_id.SetHex(request.params[0].get_str());

            // Get validator
            CValidator* validator = g_validatorRegistry.GetValidator(validator_id);
            if (!validator) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Validator not found");
            }

            // Extract destination from script
            CTxDestination dest;
            if (!ExtractDestination(validator->scriptPubKey, dest)) {
                throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to extract validator address");
            }

            // Return validator info
            UniValue result(UniValue::VOBJ);
            result.pushKV("validator_id", validator_id.ToString());
            result.pushKV("address", EncodeDestination(dest));
            result.pushKV("stake_amount", FormatMoney(validator->stakedAmount));
            result.pushKV("status", validator->status == ValidatorStatus::ACTIVE ? "active" : "slashed");
            result.pushKV("reputation", static_cast<int>(validator->reputation.reputationScore));
            result.pushKV("registration_time", validator->registrationTime);
            result.pushKV("meets_minimum_stake", validator->MeetsMinimumStake());

            return result;
        },
    };
}

static RPCHelpMan setstaking()
{
    return RPCHelpMan{"setstaking",
        "\nEnable or disable validator staking (block production).\n",
        {
            {"enabled", RPCArg::Type::BOOL, RPCArg::Optional::NO, "Enable (true) or disable (false) staking"},
        },
        RPCResult{
            RPCResult::Type::OBJ, "", "",
            {
                {RPCResult::Type::BOOL, "staking", "Current staking status"},
                {RPCResult::Type::STR, "message", "Status message"},
            }
        },
        RPCExamples{
            HelpExampleCli("setstaking", "true")
            + HelpExampleCli("setstaking", "false")
            + HelpExampleRpc("setstaking", "true")
        },
        [&](const RPCHelpMan& self, const JSONRPCRequest& request) -> UniValue
        {
            bool enable_staking = request.params[0].get_bool();
            
            // This would integrate with the PoSMiner to enable/disable block production
            // For now, we'll just return the status
            
            UniValue result(UniValue::VOBJ);
            result.pushKV("staking", enable_staking);
            result.pushKV("message", enable_staking ? "Validator staking enabled" : "Validator staking disabled");

            return result;
        },
    };
}

void RegisterValidatorRPCCommands(CRPCTable &t)
{
    static const CRPCCommand commands[]{
        {"validator", &registervalidator},
        {"validator", &removevalidator},
        {"validator", &listvalidators},
        {"validator", &getvalidatorinfo},
        {"validator", &setstaking},
    };
    for (const auto& c : commands) {
        t.appendCommand(c.name, &c);
    }
}
