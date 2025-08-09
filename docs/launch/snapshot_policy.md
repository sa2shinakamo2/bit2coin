# Snapshot Policy (Pre-Announcement)

## Rule
- The snapshot block is the first Bitcoin block whose median-time-past (MTP) is ≥ 2025-09-03 16:00:00 UTC.
- Finalization buffer: wait 144 additional Bitcoin blocks after the snapshot block before publishing the official height.

## Fairness
- Pre-announce the timestamp at least 1–2 weeks in advance.
- No blacklists/whitelists; include all addresses per Bitcoin consensus.
- Claim window: 15 months from the official snapshot publication; after this, unclaimed allocations follow the Unclaimed policy below.

## Contingency
- If Bitcoin experiences a ≥6-block reorg overlapping the snapshot window, shift to the next block with MTP ≥ <TIMESTAMP + 6 HOURS> and reapply the 144-block buffer.

## Deliverables
- Publish: exact height H, Merkle roots, and reproducible scripts to recompute balances.
- Provide: claim portal/CLI and domain-separated signing standard per `claim_spec_v1.md`.

## Unclaimed policy
- After the 15-month claim window, any unclaimed allocations are routed to a vesting contract with beneficiary set to the disclosed founder address (address-only disclosure). Vesting is enforced on-chain with a 12-month cliff and linear monthly unlocks over 48 months.
