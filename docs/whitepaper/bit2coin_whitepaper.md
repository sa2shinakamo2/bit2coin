# Bit2coin: Bitcoin‑like Proof‑of‑Stake with Fair Snapshot Claims

Version: v0.1 (Draft)

Author: Bit2coin Core (address‑only founder: `b2c1qcctsw0s2xs76fjajnan06jus2pvavvxm4l0wpa`)

## Abstract
Bit2coin is a Bitcoin‑like blockchain that replaces Proof‑of‑Work with energy‑efficient Proof‑of‑Stake. It preserves the Bitcoin user experience (secp256k1 keys, bech32 addresses, conservative issuance) while introducing: VRF‑based proposer selection, BFT‑style finality, and slashing.
Distribution is achieved via a public Bitcoin UTXO snapshot: BTC holders can claim their allocation non‑custodially by signing a message. Unclaimed coins vest to the founder address on‑chain, with no premine or privileged allocations.

## Design Goals
- Simplicity and familiarity (Bitcoin UX, conservative policy)
- Energy efficiency (replace PoW with PoS)
- Strong safety (finality + slashing)
- Fair launch (public BTC snapshot, long claim window)
- Auditability and reproducibility (open scripts, deterministic proofs)

## Snapshot and Distribution
- Timestamp: 2025‑09‑03 16:00:00 UTC.
- Snapshot rule: first Bitcoin block with MTP ≥ timestamp; publish after +144 BTC blocks.
- Claim window: 15 months.
- Claim mechanism: domain‑separated BTC message signing + Merkle proof verification.
- Unclaimed policy: unclaimed snapshot coins vest to the founder via an on‑chain vesting contract (4‑year vesting, 12‑month cliff; address‑only disclosure).

See `docs/launch/snapshot_policy.md` and `docs/launch/claim_spec_v1.md` for precise procedures.

## Consensus Overview
- Proposer selection: per‑slot VRF lottery among active validators, weighted by effective stake and subject to anti‑grinding rules.
- Finality: checkpoint‑based BFT/FFG‑style voting; justified and finalized checkpoints with slashing for safety violations (double‑vote, surround‑vote).
- Fork choice: LMD‑GHOST over the justified checkpoint, then finalize.
- Block time target: ~8s.
- Unbonding period: 21 days; warm‑up: 7 days.
- Slashing: proportional penalties; correlated events incur harsher penalties.

Details in `docs/consensus.md`.

## Economic Policy
- Scarcity similar to Bitcoin: conservative issuance; inflation declines with adoption of fees.
- Rewards: split among proposer, attesters/finality voters, and treasury (for public goods) per epoch.
- Fees: simple fee market, congestion‑responsive minimum fee; protocol fee burned or directed per governance.
- Staking economics: incentives tuned for liveness > 2/3 honest stake; penalties for inactivity and equivocation.

Details in `docs/economics.md`.

## Validator Lifecycle
- Join: bond stake; pass warm‑up before eligibility.
- Operate: produce blocks when elected; vote on checkpoints; maintain availability.
- Exit: initiate unbonding; funds unlock after unbonding window.
- Slashing conditions: double‑proposal, double‑vote, surround‑vote, and proven key compromise.

## Security and Threat Model
- Consensus safety under < 1/3 Byzantine stake; probabilistic liveness when ≥ 2/3 honest stake are online.
- VRF unpredictability limits proposer grinding; RANDAO/threshold randomness as secondary entropy.
- Long‑range attacks mitigated via finality checkpoints and weak subjectivity (light clients pin finalized states).
- Claim safety: domain‑separated messages, hardware wallet support, no on‑chain BTC spending required.

## Claim Protocol (High Level)
1. Export balances and build a deterministic Merkle tree (sorted by address; leaf = SHA256(address | "|" | balance)).
2. Publish `balances.csv`, tree root, and reproducibility kit.
3. User signs the domain‑separated message with their BTC key; submits signature + proof to claim.
4. Contract verifies signature, proof, and non‑replay protections; mints Bit2coin to the corresponding address.

See `scripts/snapshot/` for the reproducibility kit.

## Governance and Treasury
- No premine; treasury is funded by a small reward share and subject to public governance.
- Founder’s allocation is strictly from unclaimed coins and is fully vested on‑chain.
- Upgrades follow transparent proposals, review, and multi‑phase activation with safety constraints.

## Implementation Notes
- Keys: secp256k1; addresses bech32 HRP `b2c`.
- Chain ID: `bit2c-mainnet-0`.
- Tooling: Python scripts for keygen and snapshot reproducibility; claim portal/CLI planned with hardware wallet support.

## Roadmap
- Public snapshot announcement, audit of claim tooling.
- Testnet with validator onboarding and slashing simulations.
- Security reviews and mainnet launch.

## References
- `docs/consensus.md`
- `docs/economics.md`
- `docs/launch/snapshot_policy.md`
- `docs/launch/claim_spec_v1.md`
- `docs/launch/treasury_policy.md`
- `docs/launch/snapshot_day.md`
