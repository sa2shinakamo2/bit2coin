# bit2coin Consensus (Draft)

Goal: Bitcoin-like chain with PoS security, explicit finality, and much lower energy.

## High-level design
- Proposer selection: VRF-based lottery among eligible validators, stake-weighted.
- Finality: BFT-style (e.g., Tendermint-like) or FFG-like checkpoint finality with ≥2/3 voting power.
- Slashing: Double-sign and surround-vote slashing; inactivity leak under low participation.
- Weak subjectivity: Periodic checkpoints; bootstrap rules documented for nodes and exchanges.

## Validator lifecycle
- Bond stake → warm-up period (e.g., 7 days) → eligible to propose/attest.
- Unbonding delay (e.g., 21–28 days) with withdrawal queue.
- Rewards distributed each epoch to proposers and attestors; penalties for faults.

## Block structure (additions)
- Header includes: proposer address, VRF output + proof, commit of attestation set, and finality status.
- Body includes: transactions, staking ops (bond/unbond/delegate), and slash proofs.

## Fork choice
- Prefer highest finalized checkpoint; otherwise, highest justified/locked commit with tie-breaker by VRF randomness.

## Security notes
- Randomness: on-chain VRF with verifiable proofs to prevent grinding.
- DOS resistance: signature/VRF verification limits, mempool/peer scoring.
- Client diversity and audits recommended pre-mainnet.
