#!/usr/bin/env python3
"""
Build a deterministic Merkle root from a balances CSV and optionally output a proof
for a single address.

CSV format (header required):
address,balance
b2c1...,123.456789
...

Rules:
- Sort entries by address (lexicographically) for determinism.
- Leaf = SHA256(address || "|" || decimal_balance_string)
- Internal node = SHA256(left || right)
- If odd number of nodes at a level, duplicate the last node (Bitcoin-style padding) for pairing.

Usage:
  python3 scripts/snapshot/build_merkle.py --csv balances.csv [--proof-address b2c1...] [--out merkle.json]

Outputs:
- Prints JSON with {root, count}.
- If --out provided, writes a JSON file with root, count, and entries hash.
- If --proof-address provided, prints a proof JSON with leaf, index, and path (siblings from leaf up).
"""
import argparse
import csv
import hashlib
import json
from dataclasses import dataclass
from typing import List, Tuple


def sha256(b: bytes) -> bytes:
    return hashlib.sha256(b).digest()


def leaf_hash(address: str, balance: str) -> bytes:
    data = (address + '|' + balance).encode('utf-8')
    return sha256(data)


@dataclass
class Entry:
    address: str
    balance: str  # decimal string as provided


def build_leaves(entries: List[Entry]) -> List[bytes]:
    return [leaf_hash(e.address, e.balance) for e in entries]


def build_merkle_root(leaves: List[bytes]) -> Tuple[bytes, List[List[bytes]]]:
    if not leaves:
        return sha256(b''), []
    levels: List[List[bytes]] = [leaves]
    cur = leaves
    while len(cur) > 1:
        nxt: List[bytes] = []
        i = 0
        n = len(cur)
        while i < n:
            left = cur[i]
            if i + 1 < n:
                right = cur[i + 1]
            else:
                right = left  # duplicate last
            nxt.append(sha256(left + right))
            i += 2
        levels.append(nxt)
        cur = nxt
    return cur[0], levels


def proof_for_index(levels: List[List[bytes]], index: int) -> List[str]:
    """Return hex-encoded sibling path for leaf at `index`.
    levels[0] is leaves; levels[-1] is root.
    """
    path: List[str] = []
    idx = index
    for lvl in levels[:-1]:  # skip root level
        n = len(lvl)
        sibling_idx = idx ^ 1  # pair index
        if sibling_idx >= n:
            sibling_idx = idx  # duplicated last node case
        path.append(lvl[sibling_idx].hex())
        idx //= 2
    return path


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--csv', required=True, help='balances CSV (address,balance) with header')
    ap.add_argument('--proof-address', help='address to output a Merkle proof for')
    ap.add_argument('--out', help='write merkle info JSON to file')
    args = ap.parse_args()

    # Load entries
    entries: List[Entry] = []
    with open(args.csv, newline='') as f:
        rdr = csv.DictReader(f)
        if 'address' not in rdr.fieldnames or 'balance' not in rdr.fieldnames:
            raise SystemExit('CSV must have headers: address,balance')
        for row in rdr:
            addr = row['address'].strip()
            bal = row['balance'].strip()
            if not addr:
                continue
            entries.append(Entry(address=addr, balance=bal))

    # Sort deterministically by address
    entries.sort(key=lambda e: e.address)

    # Build leaves and tree
    leaves = build_leaves(entries)
    root, levels = build_merkle_root(leaves)

    result = {
        'root': root.hex(),
        'count': len(entries),
    }
    print(json.dumps(result, indent=2))

    if args.out:
        with open(args.out, 'w') as f:
            json.dump(result, f, indent=2)

    if args.proof_address:
        # find index
        idx = next((i for i, e in enumerate(entries) if e.address == args.proof_address), None)
        if idx is None:
            raise SystemExit('address not found in CSV')
        proof = {
            'address': entries[idx].address,
            'balance': entries[idx].balance,
            'leaf': leaves[idx].hex(),
            'index': idx,
            'path': proof_for_index(levels, idx),
        }
        print(json.dumps({'proof': proof}, indent=2))


if __name__ == '__main__':
    main()
