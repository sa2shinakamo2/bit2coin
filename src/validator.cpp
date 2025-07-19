// Copyright (c) 2023-2025 The BT2C developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <validator.h>
#include <random.h>
#include <util/time.h>
#include <logging.h>
#include <chainparams.h>
#include <util/moneystr.h>
#include <hash.h>

// Global validator registry instance
CValidatorRegistry g_validatorRegistry;

void ValidatorReputation::UpdateReputationScore()
{
    // Simple reputation calculation algorithm
    // Base score: 50 (neutral)
    int score = 50;
    
    // Add points for blocks produced (up to +30)
    score += std::min(blocksProduced / 10, 30u);
    
    // Subtract points for blocks missed (up to -20)
    score -= std::min(blocksMissed / 5, 20u);
    
    // Heavily penalize slashable offenses (up to -50)
    score -= std::min(slashableOffenses * 10, 50u);
    
    // Ensure score stays within 0-100 range
    reputationScore = std::max(0, std::min(score, 100));
}

void CValidator::Activate(int64_t activationTime)
{
    status = ValidatorStatus::ACTIVE;
    
    if (reputation.firstActiveTime == 0) {
        reputation.firstActiveTime = activationTime;
    }
    
    reputation.lastActiveTime = activationTime;
}

void CValidator::Deactivate(int64_t deactivationTime)
{
    if (status == ValidatorStatus::ACTIVE) {
        int64_t activeTime = deactivationTime - reputation.lastActiveTime;
        reputation.totalActiveTime += activeTime;
    }
    
    status = ValidatorStatus::INACTIVE;
}

CAmount CValidator::Slash(int64_t slashTime, double slashRatio)
{
    // Record the slashing event
    reputation.slashableOffenses++;
    reputation.UpdateReputationScore();
    
    // Calculate slashed amount (default 10% of stake)
    CAmount slashedAmount = static_cast<CAmount>(stakedAmount * slashRatio);
    
    // Update staked amount
    stakedAmount -= slashedAmount;
    
    // Update status
    status = ValidatorStatus::SLASHED;
    
    // Return the slashed amount (to be redistributed or burned)
    return slashedAmount;
}

bool CValidatorRegistry::RegisterValidator(const CScript& scriptPubKey, CAmount stakedAmount, int64_t registrationTime)
{
    LOCK(cs_validators);
    
    // Create new validator
    CValidator validator(scriptPubKey, stakedAmount);
    validator.registrationTime = registrationTime;
    
    // Check minimum stake requirement (32 BTC)
    if (!validator.MeetsMinimumStake()) {
        LogPrintf("Validator registration failed: does not meet minimum stake requirement of 32 BTC\n");
        return false;
    }
    
    // No special rewards for first validator in this project
    
    // Add to registry
    validators[validator.validatorId] = validator;
    
    LogPrintf("Validator registered with ID %s and stake amount %s\n", 
              validator.validatorId.ToString(),
              FormatMoney(stakedAmount));
    
    return true;
}

bool CValidatorRegistry::RemoveValidator(const uint256& validatorId)
{
    LOCK(cs_validators);
    
    auto it = validators.find(validatorId);
    if (it == validators.end()) {
        return false;
    }
    
    // Set status to pending exit
    it->second.status = ValidatorStatus::PENDING_EXIT;
    
    // After a cooldown period, the validator would be fully removed
    // For now, we keep it in the registry with PENDING_EXIT status
    
    LogPrintf("Validator %s set to PENDING_EXIT status\n", validatorId.ToString());
    
    return true;
}

CValidator* CValidatorRegistry::GetValidator(const uint256& validatorId)
{
    LOCK(cs_validators);
    
    auto it = validators.find(validatorId);
    if (it == validators.end()) {
        return nullptr;
    }
    
    return &it->second;
}

std::vector<CValidator*> CValidatorRegistry::GetActiveValidators()
{
    LOCK(cs_validators);
    
    std::vector<CValidator*> activeValidators;
    
    for (auto& pair : validators) {
        if (pair.second.status == ValidatorStatus::ACTIVE) {
            activeValidators.push_back(&pair.second);
        }
    }
    
    return activeValidators;
}

CValidator* CValidatorRegistry::SelectNextValidator(const uint256& prevBlockHash, int64_t timestamp)
{
    LOCK(cs_validators);
    
    std::vector<CValidator*> activeValidators;
    std::vector<int> weights;
    int totalWeight = 0;
    
    // Collect active validators and calculate weights based on stake and reputation
    for (auto& pair : validators) {
        if (pair.second.status == ValidatorStatus::ACTIVE) {
            activeValidators.push_back(&pair.second);
            
            // Weight = stake amount * reputation factor
            int reputationFactor = pair.second.reputation.reputationScore / 10; // 0-10 range
            int weight = pair.second.stakedAmount / COIN * (reputationFactor + 1); // Ensure non-zero
            
            weights.push_back(weight);
            totalWeight += weight;
        }
    }
    
    // No active validators
    if (activeValidators.empty()) {
        return nullptr;
    }
    
    // Use VRF-like selection using previous block hash and timestamp
    uint256 selectionSeed = prevBlockHash;
    // Mix in the timestamp by using CHashWriter
    CHashWriter hasher(SER_GETHASH, 0);
    hasher << selectionSeed;
    hasher << timestamp;
    selectionSeed = hasher.GetHash();
    uint64_t randomValue = GetRand(totalWeight);
    
    // Select validator based on weighted probability
    uint64_t cumulativeWeight = 0;
    for (size_t i = 0; i < activeValidators.size(); i++) {
        cumulativeWeight += static_cast<uint64_t>(weights[i]);
        if (randomValue < cumulativeWeight) {
            return activeValidators[i];
        }
    }
    
    // Fallback to last validator if something went wrong
    return activeValidators.back();
}

void CValidatorRegistry::UpdateValidatorReputation(const uint256& validatorId, bool producedBlock)
{
    LOCK(cs_validators);
    
    auto it = validators.find(validatorId);
    if (it == validators.end()) {
        return;
    }
    
    if (producedBlock) {
        it->second.reputation.blocksProduced++;
    } else {
        it->second.reputation.blocksMissed++;
    }
    
    it->second.reputation.UpdateReputationScore();
}

// No distribution period or special rewards in this project
CAmount CValidatorRegistry::GetValidatorReward(const uint256& validatorId, int64_t currentTime) const
{
    // Standard block rewards are handled by the consensus mechanism
    // No special validator rewards in this implementation
    return 0;
}
