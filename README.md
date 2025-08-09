# bit2coin

A Bitcoin-like Proof-of-Stake (PoS) chain. Keep the Bitcoin feel (secp256k1 keys, bech32-style addresses, simple UX) while replacing Proof-of-Work with energy‑efficient PoS (VRF-based proposer selection, BFT-style finality, and slashing).

This repository contains the specifications and launch documents required to reach the public snapshot announcement.

## Milestone: Pre-snapshot readiness
- Consensus + economics spec (draft)
- Snapshot policy and announcement template
- Claim Spec v1 (BTC message-signing, proofs)
- Treasury policy (transparent, on-chain vesting)
- Genesis parameter sheet (defaults)

## Quick links
- `docs/consensus.md`
- `docs/economics.md`
- `docs/launch/snapshot_policy.md`
- `docs/launch/snapshot_day.md`
- `docs/launch/claim_spec_v1.md`
- `docs/launch/treasury_policy.md`
- `docs/genesis/genesis_params.yaml`
- `ANNOUNCEMENTS/snapshot_announcement_template.md`

## Roadmap to Snapshot Announcement
1. Freeze and publish specs in this repo.
2. Announce snapshot timestamp and rule (MTP-based) per `docs/launch/snapshot_policy.md`.
3. After 144 BTC blocks, publish official height + reproducibility kit.
4. Open claim portal/CLI for BTC holders.
