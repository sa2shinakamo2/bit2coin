#!/usr/bin/env python3
"""
bit2coin offline key generator (single-sig)
- Generates a secp256k1 private key
- Derives compressed public key
- Derives a bech32 (segwit v0-style) address with HRP `b2c`

Usage:
  python3 b2c_keygen.py [--hrp b2c] [--out-prefix founder]

Outputs (stdout):
  - Private key (hex)
  - Compressed public key (hex)
  - Bech32 address

Writes (optional):
  - <out-prefix>.priv (hex)
  - <out-prefix>.pub (hex)
  - <out-prefix>.addr (text)

Keep the private key offline and secure. This tool does NOT upload or transmit keys.
"""
import os
import argparse
import hashlib
from ecdsa import SigningKey, SECP256k1
from bech32 import bech32_encode, convertbits


def sha256(b: bytes) -> bytes:
    return hashlib.sha256(b).digest()


def ripemd160(b: bytes) -> bytes:
    h = hashlib.new('ripemd160')
    h.update(b)
    return h.digest()


def hash160(b: bytes) -> bytes:
    return ripemd160(sha256(b))


def compress_pubkey(vk) -> bytes:
    # ecdsa VerifyingKey to compressed pubkey
    px = vk.pubkey.point.x()
    py = vk.pubkey.point.y()
    prefix = 0x02 | (py & 1)
    return bytes([prefix]) + px.to_bytes(32, 'big')


def segwit_bech32_address(hrp: str, witver: int, witprog: bytes) -> str:
    # Convert witness program (bytes) to 5-bit groups and encode
    data = [witver] + list(convertbits(witprog, 8, 5, True))
    return bech32_encode(hrp, data)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--hrp', default='b2c', help='address HRP (default: b2c)')
    ap.add_argument('--out-prefix', default='founder', help='prefix for output files written alongside (default: founder)')
    ap.add_argument('--no-write', action='store_true', help='do not write files; print to stdout only')
    args = ap.parse_args()

    # Generate private key
    sk = SigningKey.generate(curve=SECP256k1)
    priv = sk.to_string()  # 32 bytes
    vk = sk.get_verifying_key()
    pub_compressed = compress_pubkey(vk)

    # Address: segwit v0 style with HRP=hrp, program=HASH160(pubkey)
    witver = 0  # v0
    witprog = hash160(pub_compressed)  # 20 bytes
    addr = segwit_bech32_address(args.hrp, witver, witprog)

    priv_hex = priv.hex()
    pub_hex = pub_compressed.hex()

    print('PrivateKey(hex):', priv_hex)
    print('PubKeyCompressed(hex):', pub_hex)
    print('Address:', addr)

    if not args.no_write:
        base = args.out_prefix
        with open(f'{base}.priv', 'w') as f:
            f.write(priv_hex + '\n')
        with open(f'{base}.pub', 'w') as f:
            f.write(pub_hex + '\n')
        with open(f'{base}.addr', 'w') as f:
            f.write(addr + '\n')
        print(f'Wrote {base}.priv, {base}.pub, {base}.addr in current directory')


if __name__ == '__main__':
    main()
