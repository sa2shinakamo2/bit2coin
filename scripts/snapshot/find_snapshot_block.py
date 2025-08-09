#!/usr/bin/env python3
"""
Find the snapshot block according to the MTP rule:
- Snapshot block = first Bitcoin block whose mediantime >= TARGET_TIMESTAMP (UTC seconds)

Requires a running bitcoind with RPC exposed.

Usage:
  python3 scripts/snapshot/find_snapshot_block.py --timestamp "2025-09-03 16:00:00" \
      --rpc-url http://127.0.0.1:8332 --rpc-user USER --rpc-pass PASS

Notes:
- TARGET_TIMESTAMP is parsed as UTC. You can also pass --timestamp-seconds <int>.
- We do a bounded search: find [low, high] such that mediantime(low) < ts <= mediantime(high),
  then binary search to the first height meeting the condition.
- Print JSON with: {"height": H, "hash": ..., "mediantime": ..., "timestamp_utc": "..."}
"""
import argparse
import datetime as dt
import json
import os
import sys
from decimal import Decimal

from bitcoinrpc.authproxy import AuthServiceProxy


def parse_args():
    ap = argparse.ArgumentParser()
    ap.add_argument('--timestamp', help='UTC timestamp, e.g. 2025-09-03 16:00:00')
    ap.add_argument('--timestamp-seconds', type=int, help='Unix epoch seconds (UTC)')
    ap.add_argument('--rpc-url', default=os.getenv('BTC_RPC_URL', 'http://127.0.0.1:8332'))
    ap.add_argument('--rpc-user', default=os.getenv('BTC_RPC_USER'))
    ap.add_argument('--rpc-pass', default=os.getenv('BTC_RPC_PASS'))
    ap.add_argument('--max-step', type=int, default=2016, help='max step when expanding search window (default 2016)')
    return ap.parse_args()


def to_epoch_seconds(ts_str: str) -> int:
    # Interpret as UTC
    return int(dt.datetime.strptime(ts_str, '%Y-%m-%d %H:%M:%S').replace(tzinfo=dt.timezone.utc).timestamp())


def get_block_by_height(rpc, h: int):
    bh = rpc.getblockhash(h)
    return rpc.getblock(bh)


def mediantime_at_height(rpc, h: int) -> int:
    blk = get_block_by_height(rpc, h)
    return int(blk['mediantime'])


def find_bounds(rpc, ts: int, max_step: int):
    tip = rpc.getblockcount()
    # If tip mediantime < ts, wait until chain advances
    tip_mt = mediantime_at_height(rpc, tip)
    if tip_mt < ts:
        raise SystemExit(f"Chain mediantime at tip {tip} is {tip_mt}, below target {ts}. Try again later.")

    # Find a low height with mediantime < ts
    low = max(0, tip - max_step)
    while low > 0:
        mt = mediantime_at_height(rpc, low)
        if mt < ts:
            break
        # expand window backwards
        new_low = max(0, low - max_step)
        if new_low == low:
            break
        low = new_low
    # Ensure low satisfies mt(low) < ts if possible
    while low < tip and mediantime_at_height(rpc, low) >= ts:
        low = max(0, low - max_step)
        if low == 0:
            break

    # Find a high height with mediantime >= ts
    high = low + max_step
    if high > tip:
        high = tip
    while mediantime_at_height(rpc, high) < ts and high < tip:
        high = min(tip, high + max_step)
    return low, high


def binary_search_first_at_least(rpc, ts: int, low: int, high: int) -> int:
    # Invariant: mediantime(low) < ts <= mediantime(high)
    while low + 1 < high:
        mid = (low + high) // 2
        mt = mediantime_at_height(rpc, mid)
        if mt >= ts:
            high = mid
        else:
            low = mid
    return high


def main():
    args = parse_args()
    if not args.timestamp and args.timestamp_seconds is None:
        print('Error: provide --timestamp or --timestamp-seconds', file=sys.stderr)
        sys.exit(2)
    ts = args.timestamp_seconds if args.timestamp_seconds is not None else to_epoch_seconds(args.timestamp)

    if not args.rpc_user or not args.rpc_pass:
        print('Error: RPC creds required (set --rpc-user/--rpc-pass or BTC_RPC_USER/BTC_RPC_PASS)', file=sys.stderr)
        sys.exit(2)

    rpc = AuthServiceProxy(args.rpc_url, username=args.rpc_user, password=args.rpc_pass, timeout=60)

    low, high = find_bounds(rpc, ts, args.max_step)
    # Ensure bounds invariant
    if mediantime_at_height(rpc, low) >= ts:
        # move one step back if possible
        low = max(0, low - 1)
    if mediantime_at_height(rpc, high) < ts:
        # move forward to tip
        tip = rpc.getblockcount()
        high = tip
        if mediantime_at_height(rpc, high) < ts:
            raise SystemExit('Could not find high bound with mediantime >= target; try again later.')

    h = binary_search_first_at_least(rpc, ts, low, high)
    blk = get_block_by_height(rpc, h)
    out = {
        'height': h,
        'hash': blk['hash'],
        'mediantime': int(blk['mediantime']),
        'timestamp_utc': dt.datetime.utcfromtimestamp(int(blk['mediantime'])).strftime('%Y-%m-%d %H:%M:%S UTC')
    }
    print(json.dumps(out, indent=2))


if __name__ == '__main__':
    main()
