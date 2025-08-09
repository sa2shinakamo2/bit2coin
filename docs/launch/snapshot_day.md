# Snapshot Day Runbook

This guide documents the exact steps to execute the snapshot, per the MTP rule.

## Prerequisites
- A fully synced `bitcoind` node with RPC enabled.
- RPC credentials and URL (e.g., `BTC_RPC_URL`, `BTC_RPC_USER`, `BTC_RPC_PASS`).
- Python 3.10+.

## 1) Find the snapshot block (after timestamp has passed)

Install requirements:
```bash
pip install -r scripts/snapshot/requirements.txt
```

Locate the snapshot block by MTP:
```bash
python3 scripts/snapshot/find_snapshot_block.py \
  --timestamp "2025-09-03 16:00:00" \
  --rpc-url http://127.0.0.1:8332 \
  --rpc-user <USER> \
  --rpc-pass <PASS>
```

Tips:
- You can set env vars instead of flags: `BTC_RPC_URL`, `BTC_RPC_USER`, `BTC_RPC_PASS`.
- Verify the output `mediantime` ≥ the timestamp.

Save the JSON output to file:
```bash
python3 scripts/snapshot/find_snapshot_block.py --timestamp "2025-09-03 16:00:00" \
  --rpc-url http://127.0.0.1:8332 --rpc-user <USER> --rpc-pass <PASS> \
  > snapshot_block.json
```

Then wait 144 additional Bitcoin blocks before publishing the snapshot as “final.”
You can estimate time as ~24 hours, or track blocks remaining from tip.

## 2) Build Merkle root (after exporting balances CSV)

Export `balances.csv` (format: `address,balance` with header). This export must be reproducible and published alongside the scripts. If you are using an external exporter, record its version and command.

```bash
python3 scripts/snapshot/build_merkle.py --csv balances.csv --out merkle.json
```

## Optional: Generate a single-address proof

```bash
python3 scripts/snapshot/build_merkle.py --csv balances.csv --proof-address <b2c1...>
```

## 3) Publish artifacts (after 144-block buffer)

Publish the following in the repo (and on your website):
- `snapshot_block.json` (height, hash, mediantime UTC)
- `balances.csv` (or partitioned files) and its SHA256
- `merkle.json` (root and count) and its SHA256
- The exact version of scripts used and commands run

Example to compute file hashes:
```bash
shasum -a 256 balances.csv merkle.json snapshot_block.json
```
