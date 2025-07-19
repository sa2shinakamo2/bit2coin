// Copyright (c) 2023-2025 The BT2C developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifndef BT2C_VALIDATOR_H
#define BT2C_VALIDATOR_H

#include <consensus/amount.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <uint256.h>
#include <sync.h>
#include <hash.h>
#include <map>
#include <vector>
#include <validator_status.h>

/** 
 * Validator system for BT2C
 * 
 * This implements the validator-based Proof of Stake system for BT2C
 * with minimum stake requirements, reputation tracking, and slashing conditions.
 */

/** Minimum stake amount required to become a validator (32 BTC) */
static const CAmount VALIDATOR_MIN_STAKE = 32 * COIN;

// No special rewards or distribution period for this project

/**
 * Validator reputation metrics
 * Used to track validator performance over time
 */
struct ValidatorReputation {
    // Performance metrics
    uint32_t blocksProduced;          // Number of blocks successfully produced
    uint32_t blocksMissed;            // Number of blocks missed when selected
    uint32_t slashableOffenses;       // Number of detected slashable offenses
    
    // Derived reputation score (0-100)
    int16_t reputationScore;
    
    // Time-based metrics
    int64_t firstActiveTime;          // First time this validator became active
    int64_t lastActiveTime;           // Last time this validator was active
    int64_t totalActiveTime;          // Total time active as a validator
    
    ValidatorReputation() : 
        blocksProduced(0),
        blocksMissed(0),
        slashableOffenses(0),
        reputationScore(50),  // Start at neutral score
        firstActiveTime(0),
        lastActiveTime(0),
        totalActiveTime(0) {}
    
    // Calculate reputation score based on performance metrics
    void UpdateReputationScore();
};

/**
 * Validator entry
 * Contains all information about a validator
 */
class CValidator {
public:
    CScript scriptPubKey;             // The validator's public key script
    CAmount stakedAmount;             // Amount of BT2C staked by this validator
    ValidatorStatus status;           // Current validator status
    ValidatorReputation reputation;   // Reputation metrics
    int64_t registrationTime;         // Time when validator registered
    uint256 validatorId;              // Unique validator identifier
    
    CValidator() : 
        stakedAmount(0),
        status(ValidatorStatus::INACTIVE),
        registrationTime(0) {}
    
    CValidator(const CScript& scriptPubKeyIn, CAmount stakedAmountIn) :
        scriptPubKey(scriptPubKeyIn),
        stakedAmount(stakedAmountIn),
        status(ValidatorStatus::INACTIVE),
        registrationTime(0) {
        // Generate validator ID from script pubkey
        // Use HashWriter directly to avoid MakeUCharSpan compatibility issues
        CHashWriter hasher(SER_GETHASH, 0);
        hasher << scriptPubKey;
        validatorId = hasher.GetHash();
    }
    
    // Check if validator meets minimum stake requirement
    bool MeetsMinimumStake() const {
        return stakedAmount >= VALIDATOR_MIN_STAKE;
    }
    
    // Activate validator
    void Activate(int64_t activationTime);
    
    // Deactivate validator
    void Deactivate(int64_t deactivationTime);
    
    // Slash validator for malicious behavior
    CAmount Slash(int64_t slashTime, double slashRatio = 0.1);
};

/**
 * Validator registry
 * Manages the set of validators in the network
 */
class CValidatorRegistry {
private:
    std::map<uint256, CValidator> validators;
    mutable RecursiveMutex cs_validators;
    
    // No special distribution period for this project
    
public:
    CValidatorRegistry() {}
    
    // Register a new validator
    bool RegisterValidator(const CScript& scriptPubKey, CAmount stakedAmount, int64_t registrationTime);
    
    // Remove a validator
    bool RemoveValidator(const uint256& validatorId);
    
    // Get validator by ID
    CValidator* GetValidator(const uint256& validatorId);
    
    // Get active validators
    std::vector<CValidator*> GetActiveValidators();
    
    // Select validator for next block using VRF
    CValidator* SelectNextValidator(const uint256& prevBlockHash, int64_t timestamp);
    
    // Update validator reputation
    void UpdateValidatorReputation(const uint256& validatorId, bool producedBlock);
    
    // No distribution period in this project
    
    // Get validator reward
    CAmount GetValidatorReward(const uint256& validatorId, int64_t currentTime) const;
    
    // No distribution period in this project
};

// Global validator registry
extern CValidatorRegistry g_validatorRegistry;

#endif // BT2C_VALIDATOR_H
