# BT2C Validator Guide

## Introduction

This document provides guidance on how to operate a validator node in the BT2C network. BT2C is a **pure Proof of Stake (PoS)** blockchain, meaning that all block production and validation is performed exclusively through staking, without any Proof of Work mining.

## Requirements

To run a BT2C validator node, you need:

1. **Minimum Stake**: 32 BT2C in your wallet
2. **Hardware**:
   - CPU: 2+ cores
   - RAM: 4+ GB
   - Storage: 100+ GB SSD
   - Bandwidth: Stable internet connection with at least 5 Mbps
3. **Software**:
   - BT2C Core software (latest version)
   - Operating System: Linux (recommended), macOS, or Windows

## Setting Up a Validator Node

### Step 1: Install BT2C Core

Download and install the latest version of BT2C Core from the official repository:

```bash
git clone https://github.com/yourusername/bt2c.git
cd bt2c
./autogen.sh
./configure
make
make install
```

### Step 2: Configure Your Node

Create or edit your `bt2c.conf` file:

```bash
mkdir -p ~/.bt2c
nano ~/.bt2c/bt2c.conf
```

Add the following configuration:

```
# Network settings
listen=1
server=1

# Validator settings
staking=1
minting=1

# Connection settings
maxconnections=50

# RPC settings (optional, for remote control)
rpcuser=yourusername
rpcpassword=REPLACE_WITH_SECURE_PASSWORD
rpcallowip=127.0.0.1
```

### Step 3: Start Your Node

Start the BT2C daemon:

```bash
bt2cd -daemon
```

### Step 4: Fund Your Wallet

Transfer at least 32 BT2C to your wallet. You can create a new address with:

```bash
bt2c-cli getnewaddress
```

### Step 5: Register as a Validator

Once your wallet is fully synced and has the minimum required balance:

```bash
bt2c-cli registervalidator
```

## Validator States

Validators in the BT2C network can be in one of the following states:

1. **Active**: Fully participating in block validation and eligible for rewards
2. **Inactive**: Registered but not actively participating
3. **Jailed**: Temporarily suspended due to missed blocks or minor protocol violations
4. **Tombstoned**: Permanently banned due to serious protocol violations

## Validator Operations

### Checking Validator Status

```bash
bt2c-cli validatorinfo
```

### Checking Staking Status

```bash
bt2c-cli getstakinginfo
```

### Withdrawing from Validator Role

```bash
bt2c-cli withdrawvalidator
```

## Security Best Practices

1. **Separate Keys**: Use separate keys for validator operations and fund storage
2. **Firewall Configuration**: Only open necessary ports
3. **Regular Updates**: Keep your node software updated
4. **Monitoring**: Set up monitoring for your validator's performance and uptime
5. **Backup**: Regularly backup your wallet.dat file and validator keys

## Troubleshooting

### Validator Not Producing Blocks

1. Check if your node is fully synced: `bt2c-cli getblockcount`
2. Verify staking is enabled: `bt2c-cli getstakinginfo`
3. Ensure you have the minimum required stake: 32 BT2C
4. Check validator status: `bt2c-cli validatorinfo`

### Validator Jailed

If your validator has been jailed:

1. Check the reason: `bt2c-cli validatorinfo`
2. Wait for the unjail period to complete
3. Request unjailing: `bt2c-cli unjailvalidator`

## Rewards and Economics

- Block rewards start at 50 BT2C and halve every 210,000 blocks
- Validators receive block rewards for producing blocks
- Transaction fees are also awarded to the block producer

## Further Resources

- [BT2C Economic Model](economic-model.md)
- [PoS Consensus Mechanism](pos-consensus.md)
- [API Reference](api-reference.md)
- [Network Parameters](network-parameters.md)
