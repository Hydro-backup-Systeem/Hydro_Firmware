import binascii
from Crypto.Cipher import AES
from Crypto.Util import Counter

# 16‐byte AES key (must match your STM32)
KEY_STRING = 'C939CC13397C1D37DE6AE0E1CB7C423C'
KEY = bytes.fromhex(KEY_STRING)

# ─── Drop your IV and ciphertext here ────────────────────────────────────────
# Hex‐encoded 16‑byte IV from STM32:
iv_hex         = '15e284e51378cb405f33cd3591bd31d7'  

# Hex‐encoded ciphertext (as one continuous string):
ciphertext_hex = 'b68a25b5aba536853a43d8cc78cbe744'
# ─────────────────────────────────────────────────────────────────────────────

def decrypt(iv_hex: str, ciphertext_hex: str) -> bytes:
    iv         = binascii.unhexlify(iv_hex)
    ciphertext = binascii.unhexlify(ciphertext_hex)

    # Build a 128‑bit big‑endian counter from your IV
    ctr = Counter.new(128, initial_value=int.from_bytes(iv, byteorder='big'))
    cipher = AES.new(KEY, AES.MODE_CTR, counter=ctr)

    # Decrypt and strip PKCS#7 padding
    pt = cipher.decrypt(ciphertext)
    print(pt)

    pad = pt[-1]
    if 1 <= pad <= AES.block_size:
        pt = pt[:-pad]
    return pt

if __name__ == '__main__':
    pt = decrypt(iv_hex, ciphertext_hex)
    print("Raw bytes  :", pt)
    print("Plaintext  :", pt.decode('utf-8', errors='replace'))
