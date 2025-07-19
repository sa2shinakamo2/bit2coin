# BT2C Economic Model

## Overview

BT2C combines Bitcoin's economic model with a pure Proof of Stake (PoS) consensus mechanism. This document outlines the economic parameters and incentive structure of the BT2C network.

## Key Economic Parameters

### Supply and Issuance

- **Maximum Supply**: 21,000,000 BT2C
- **Initial Block Reward**: 21.0 BT2C
- **Reward Halving**: Every 4 years (126,144,000 seconds)
- **Minimum Reward**: 0.00000001 BT2C (1 satoshi)

### Block Production

- **Target Block Time**: 60 seconds
- **Block Size**: Similar to Bitcoin's block structure
- **Transaction Fees**: Dynamic fee market

### Validator Economics

- **Minimum Stake**: 1.0 BT2C required to become a validator
- **Early Validator Reward**: 1.0 BT2C bonus for early validators
- **Developer Node Reward**: 100 BT2C allocated to developer nodes
- **Distribution Period**: 14 days (ending April 21, 2025)

## Block Rewards

BT2C follows a Bitcoin-like block reward schedule but with faster block times:

| Period | Block Range | Reward per Block |
|--------|-------------|------------------|
| Initial | 0 - 2,102,400 | 21.0 BT2C |
| First Halving | 2,102,401 - 4,204,800 | 10.5 BT2C |
| Second Halving | 4,204,801 - 6,307,200 | 5.25 BT2C |
| Third Halving | 6,307,201 - 8,409,600 | 2.625 BT2C |
| And so on... | ... | ... |

The reward continues halving until it reaches the minimum of 1 satoshi (0.00000001 BT2C).

## Validator Incentives

In BT2C's pure PoS system, validators are incentivized to maintain honest behavior through:

1. **Block Rewards**: Validators receive block rewards for producing valid blocks
2. **Transaction Fees**: All transaction fees in a block go to the block producer
3. **Reputation System**: Higher reputation leads to more frequent block production opportunities
4. **Slashing Risks**: Dishonest behavior results in stake penalties

## Fee Market

BT2C implements a dynamic fee market similar to Bitcoin:

- **Base Fee**: Minimum fee required for transaction inclusion
- **Priority Fee**: Optional fee to prioritize transaction processing
- **Fee Estimation**: Wallet software estimates appropriate fees based on network congestion

## Distribution Period

BT2C launched with a 14-day distribution period (ending April 21, 2025) with special economic parameters:

- **Early Validator Rewards**: Additional 1.0 BT2C per block for validators who join during this period
- **Developer Node Allocation**: 100 BT2C allocated to developer nodes to bootstrap the network
- **Network Bootstrap**: Initial parameters optimized for network growth

## Current Network State

As of the latest update:

- **Block Height**: ~392 blocks
- **Active Validators**: Single validator network (early stage)
- **Developer Node Wallet**: bt2c_4k3qn2qmiwjeqkhf44wtowxb
- **Standalone Wallet**: bt2c_tl6wks4nrylrznhmwiepo4wj

## Economic Security

BT2C's economic model is designed to ensure network security through:

1. **Minimum Stake Requirement**: Ensures validators have skin in the game
2. **Slashing Penalties**: Economic disincentives for malicious behavior
3. **Long-term Value Alignment**: Validators are BT2C holders with interest in network success
4. **Scarcity Model**: Fixed maximum supply creates long-term value proposition

## Comparison to Bitcoin's Economic Model

BT2C maintains Bitcoin's core economic principles while adapting them for PoS:

| Parameter | Bitcoin | BT2C |
|-----------|---------|------|
| Maximum Supply | 21,000,000 BTC | 21,000,000 BT2C |
| Initial Block Reward | 50 BTC | 21 BT2C |
| Halving Schedule | Every 210,000 blocks (~4 years) | Every 4 years (126,144,000 seconds) |
| Minimum Unit | 1 satoshi (0.00000001 BTC) | 1 satoshi (0.00000001 BT2C) |
| Consensus Mechanism | Proof of Work | Pure Proof of Stake |
| Block Time | 10 minutes | 60 seconds |
| Mining/Staking Requirement | Specialized hardware | 1.0 BT2C minimum stake |

## Future Economic Considerations

The BT2C development team is considering several economic improvements:

1. **Delegated Staking**: Allow users with less than the minimum stake to participate
2. **Governance Treasury**: Small allocation of block rewards for ecosystem development
3. **Enhanced Fee Mechanisms**: More sophisticated fee market for improved user experience
4. **Cross-chain Bridges**: Economic mechanisms for interoperability with other blockchains

## References

- [BT2C Whitepaper](https://bt2c.net/whitepaper.pdf)
- [Validator Guide](validator-guide.md)
- [PoS Consensus](pos-consensus.md)
- [API Reference](api-reference.md)
