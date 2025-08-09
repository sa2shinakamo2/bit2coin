# Claim Spec v1 (BTC → bit2coin)

## Overview
BTC holders can claim bit2coin allocations non-custodially by proving ownership of BTC addresses at snapshot height H.

## Supported address types
- Legacy (P2PKH), P2SH, SegWit (P2WPKH/P2WSH), Taproot (P2TR).

## Proof of ownership
- Sign a domain-separated message with the BTC private key corresponding to the address.
- Suggested message format (ASCII):
  - "bit2coin-claim:v1|chain-id=<BIT2COIN_CHAIN_ID>|claim-to=<BIT2COIN_ADDR>|btc-addr=<BTC_ADDR>|nonce=<N>"
- The signature is verified against the BTC address type rules.

## Anti-replay
- Domain separation via the fixed prefix and chain-id.
- Nonce prevents cross-claim reuse.

## Claim flow
1. User inputs BTC address and target bit2coin address.
2. Tool generates the exact message; user signs via hardware wallet/CLI.
3. Submit signature + Merkle proof of balance to the claim contract/module.
4. Module mints or unlocks allocation to the bit2coin address; records that BTC address as claimed.

## Merkle proofs
- Snapshot computation maps each BTC address to its balance; tree root published.
- Claim includes path proof verifying the entry against the published root.

## Safety guidelines
- Never request a spend transaction, only message signatures.
- Publish open-source CLI and audited web app; provide test vectors.
