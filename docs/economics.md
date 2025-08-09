# bit2coin Economics (Draft)

## Supply and issuance
- Target ethos: Bitcoin-like scarcity. Options:
  - Fixed cap mapped from BTC snapshot + vesting and rewards drawn from fees and small tail emission; or
  - Conservative tail emission (e.g., ≤1%/yr) to sustain security budget.

## Rewards and penalties
- Reward split: proposer:attestors = 1:9 (tunable).
- Target participation: 67–75%.
- Slashing: 1–2% for double-sign; 0.5–1% for surround; inactivity leak up to 5%/month when participation < 2/3.

## Parameters (defaults; see genesis params)
- Block time: 5–10s.
- Unbonding: 21–28 days.
- Warm-up: 7 days.
- Optional treasury fee on rewards (2%) for first 24 months; auto-sunset.
