# BT2C Proof of Stake Consensus

## Overview

BT2C implements a **pure Proof of Stake (PoS)** consensus mechanism. Unlike hybrid cryptocurrencies that combine PoW and PoS, BT2C has completely removed all Proof of Work mining code paths, making it exclusively a PoS blockchain. This document outlines the technical details of BT2C's PoS consensus mechanism.

## Key Components

### 1. Validator System

BT2C's consensus relies on validators who stake a minimum amount of coins to participate in block production:

- **Minimum Stake**: 32 BT2C required to become a validator
- **Validator Registry**: On-chain registry of active validators
- **Reputation System**: Tracks validator performance and reliability

### 2. Block Production

Blocks in BT2C are produced exclusively through staking:

- **Block Time**: 60 seconds target time
- **Block Size**: Similar to Bitcoin's block structure
- **Block Rewards**: Start at 21 BT2C per block, halving every 4 years (126,144,000 seconds)

### 3. Validator Selection

For each block, a validator is selected through a deterministic but unpredictable process:

- **Selection Algorithm**: Uses a VRF-like (Verifiable Random Function) mechanism
- **Selection Factors**:
  - Stake amount (minimum 32 BT2C)
  - Validator reputation score
  - Time since last block produced
  - Randomization factor derived from previous block hash

### 4. Consensus Rules

BT2C enforces strict consensus rules to maintain network security:

- **PoW Blocks Rejected**: All PoW blocks are automatically rejected by the network
- **PoS Validation**: Blocks must contain valid PoS proofs
- **Chain Selection**: Longest valid chain with highest cumulative stake weight
- **Finality**: Blocks are considered final after 6 confirmations

### 5. Security Mechanisms

To prevent attacks and ensure network integrity:

- **Slashing**: Penalties for validators who violate protocol rules
- **Jailing**: Temporary suspension for validators who miss blocks
- **Tombstoning**: Permanent ban for validators who attempt double-signing
- **Nothing-at-Stake Prevention**: Validators lose stake for contributing to multiple chains

## Technical Implementation

### Block Validation

When a new block is received, BT2C nodes perform the following validation steps:

1. **Header Validation**: Verify block header format and signature
2. **PoW Check**: Reject any block attempting to use PoW (all PoW blocks are invalid)
3. **PoS Verification**: Validate the PoS proof and stake kernel
4. **Validator Eligibility**: Confirm the block producer is an eligible validator
5. **Transaction Validation**: Verify all transactions in the block
6. **Consensus Rules**: Ensure the block follows all consensus rules

### Stake Kernel

The stake kernel is the core component of BT2C's PoS system:

```cpp
// Simplified stake kernel verification
bool CheckStakeKernel(
    const CBlockIndex* pindexPrev,
    unsigned int nBits,
    uint256 blockHash,
    uint256 stakeTxHash,
    unsigned int nTime,
    unsigned int nTimeBlockFrom)
{
    // Verify stake transaction
    // Verify stake age
    // Verify stake modifier
    // Calculate hit value and check against target
    // Additional BT2C-specific validations
}
```

### Validator Registration

Validators register on-chain with a special transaction type:

```cpp
// Simplified validator registration
bool RegisterValidator(
    const CAmount& stakeAmount,
    const CScript& validatorScript,
    const CPubKey& validatorPubKey)
{
    // Verify minimum stake (32 BT2C)
    if (stakeAmount < MINIMUM_VALIDATOR_STAKE)
        return false;
        
    // Create validator registration transaction
    // Add to validator registry
    // Initialize reputation score
    
    return true;
}
```

## Differences from Peercoin's Hybrid Model

BT2C has made significant changes to Peercoin's original hybrid PoW/PoS model:

1. **Complete Removal of PoW**:
   - All PoW mining code paths have been removed or disabled
   - Block validation explicitly rejects PoW blocks
   - Mining RPCs return appropriate values for a PoS-only system

2. **Enhanced Validator System**:
   - Increased minimum stake (32 BT2C vs Peercoin's lower requirement)
   - Added reputation-based validator selection
   - Implemented slashing for protocol violations

3. **Economic Model Adjustments**:
   - Modified block rewards to match Bitcoin's model (21 BT2C initial)
   - Implemented Bitcoin-style halving schedule
   - Maintained 21 million maximum supply

## Network Parameters

BT2C's PoS consensus is configured with the following network parameters:

- **Block Time**: 60 seconds
- **Minimum Stake**: 32 BT2C
- **Seed Nodes**:
  - seed1.bt2c.net:26656
  - seed2.bt2c.net:26656
- **Default Validator Port**: 26656
- **Metrics Port**: 26660

## Future Improvements

The BT2C development team is considering several improvements to the PoS consensus:

1. **Delegated Staking**: Allow users with less than 32 BT2C to participate
2. **Governance Module**: On-chain voting for protocol upgrades
3. **Enhanced Finality**: Implement stronger finality guarantees
4. **Validator Committees**: Rotating committees for improved scalability

## References

- [BT2C Whitepaper](https://bt2c.net/whitepaper.pdf)
- [Validator Guide](validator-guide.md)
- [Economic Model](economic-model.md)
- [API Reference](api-reference.md)
