# bit2coin Snapshot Announcement (Template)

We will take the Bitcoin snapshot at: 2025-09-03 16:00:00 UTC.

Rule:
- Snapshot block = first Bitcoin block with median-time-past (MTP) ≥ 2025-09-03 16:00:00 UTC.
- We will wait 144 additional Bitcoin blocks before publishing the official snapshot height and Merkle roots.

Why:
- To enable fair, non-custodial claims for BTC holders on bit2coin (a Bitcoin-like PoS chain) while ensuring transparency and reorg safety.

What’s next:
- Within 24 hours after the 144-block buffer, we’ll publish:
  - The exact height H and Merkle root(s)
  - Reproducible scripts to recompute balances
  - Claim Spec v1 and tooling (web portal + CLI)
  - Founder vesting destination address (address-only disclosure) and vesting schedule
    - Founder address: b2c1qcctsw0s2xs76fjajnan06jus2pvavvxm4l0wpa

Security & Safety:
- Claims require only message signatures (no BTC spends).
- Hardware wallet flows supported.
- Domain-separated messages; anti-replay protections.

Unclaimed policy:
- After a 15-month claim window, any unclaimed allocations will be routed to a vesting contract with beneficiary set to the disclosed founder address. Vesting is enforced on-chain (cliff + linear unlocks). No personal identity is disclosed; address-only.

Links:
- Snapshot Policy: `docs/launch/snapshot_policy.md`
- Claim Spec v1: `docs/launch/claim_spec_v1.md`
- Treasury Policy: `docs/launch/treasury_policy.md`
