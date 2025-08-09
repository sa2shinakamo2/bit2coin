# Founder Vesting Contract (Spec Draft)

Purpose: hold all unclaimed snapshot allocations and release them to the founder over time with a cliff and linear vesting. Enforces address-only disclosure (no identity needed), and no early transfers.

## Parameters
- beneficiary: founder destination address (single-sig or multisig)
- start_time (genesis or T+X)
- cliff_months: 12 (proposed)
- total_months: 48 (proposed)
- token: bit2coin native token
- admin: none (immutable) or time-locked governance to pause only on critical consensus incident (optional)

## Behavior
- Before `start_time + cliff`, no withdrawals allowed.
- After cliff, linear monthly (or per-block) unlock proportional to elapsed time until full vest at `start_time + total_months`.
- Non-transferable escrow: tokens cannot be reassigned or used for governance until withdrawn to beneficiary.
- No mint/burn capability; contract only holds and releases pre-allocated tokens.
- Event logs: `Vested(amount, timestamp)`, `Withdraw(amount, timestamp, beneficiary)`.

## Security
- Immutable parameters after deployment (constructor-only set).
- No upgradability to avoid governance capture; if upgradeable is required, use a time-lock + multi-sig with a long delay and public announcement.
- Independent audits required before mainnet.

## Genesis Wiring
- At snapshot claim window end, route all unclaimed allocation to this contract.
- Publish address and parameters in genesis docs and announcement post-snapshot.

## Minimal Interface (pseudocode)
```
contract FounderVesting {
  address beneficiary;
  uint64 start_time;
  uint32 cliff_seconds;
  uint32 total_seconds;
  uint256 total_alloc;
  uint256 released;

  function releasable(now) returns (uint256)
  function release(to=beneficiary) // transfers `releasable` and updates `released`
}
```

Notes: Implementation details depend on the chosen stack (Cosmos SDK module, Substrate pallet, or smart-contract VM). This spec defines the required semantics irrespective of stack.
