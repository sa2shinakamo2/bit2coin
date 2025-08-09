# bit2coin Offline Keygen

Generate a founder address offline. This script never transmits keys.

## 1) Create a virtualenv (recommended)
```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r scripts/keygen/requirements.txt
```

## 2) Generate keys and address
```bash
python3 scripts/keygen/b2c_keygen.py --hrp b2c --out-prefix founder
```
Outputs (and writes files in current dir):
- PrivateKey(hex): <hex>
- PubKeyCompressed(hex): <hex>
- Address: b2c1...
- Files: `founder.priv`, `founder.pub`, `founder.addr`

Use `--no-write` to avoid creating files (prints only).

## 3) Secure the private key
- Move `founder.priv` to an encrypted, offline medium.
- Do NOT commit keys to git.

## 4) Share only the address
- Send `founder.addr` (the bech32 address) so it can be placed into:
  - `docs/genesis/genesis_params.yaml` (founder_vesting.address)
  - `docs/launch/treasury_policy.md` (<FOUNDER_VESTING_ADDRESS>)
  - `ANNOUNCEMENTS/snapshot_announcement_template.md` (address-only disclosure)

## Notes
- HRP defaults to `b2c`. Change with `--hrp` if we adopt a different prefix.
- This uses SegWit v0-style bech32 formatting with HASH160(pubkey) as the witness program.
- For multisig or hardware wallet flows, we will extend tooling later.
