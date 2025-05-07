import time
import serial
import binascii
from Crypto.Cipher import AES

# Make sure this key matches the one used on the STM32 side.
# This example uses a 16-byte key for AES-128.
KEY_STRING = 'C939CC13397C1D37DE6AE0E1CB7C423C'  # Replace with your actual key
KEY = bytes.fromhex(KEY_STRING)

def fix_endianness(decrypted_bytes):
  # Swap bytes in each 4-byte chunk
  fixed_bytes = bytearray()
  for i in range(0, len(decrypted_bytes), 4):
    fixed_bytes.extend(decrypted_bytes[i:i+4][::-1])  # Reverse every 4-byte block
  return fixed_bytes

def main():
  # Open serial port; adjust '/dev/ttyACM0' and baud rate as needed.
  ser = serial.Serial('/dev/ttyACM0', 115200, timeout=1)
  
  while True:
    try:
      # Read a line and check if it's the start of a message
      line = ser.readline().decode('utf-8').strip()
      if line.startswith("Plaintext:"):
        # Read expected plaintext from the next line
        expected_plaintext = ser.readline().decode('utf-8').strip()

        # Read the "InitVec:" label (and ignore it)
        while True:
          label = ser.readline().decode('utf-8').strip()
          print(f'Label: {label}')
          if not label.startswith("InitVec"):
            continue  # out of sync; try next message
          break
        
        time.sleep(0.01)  # Small delay to stabilize UART buffer
        # Read the actual IV in hex
        raw_iv = ser.readline()
        iv_hex = raw_iv.decode('utf-8').strip()
        print(f"Raw IV received: {raw_iv} converted => {iv_hex}")  # Shows raw data, including \n or \r

        time.sleep(0.01)  # Small delay to stabilize UART buffer
        # Read the "Encrypted" label (and ignore it)
        while True:
          label = ser.readline().decode('utf-8').strip()
          print(f'Label: {label}')
          if not label.startswith("Encrypted"):
            continue  # out of sync; try next message
          break
        
        time.sleep(0.01)  # Small delay to stabilize UART buffer
        # Read the ciphertext in hex
        ciphertext_hex_raw = ser.readline()
        ciphertext_hex = ciphertext_hex_raw.decode('utf-8').strip()
        print(f"Raw ciphertext received: {ciphertext_hex_raw} converted => {ciphertext_hex}")

        # Convert hex strings to bytes using binascii
        try:
          ciphertext = binascii.unhexlify(ciphertext_hex)
        except binascii.Error as e:
          print(f"Error in hex conversion (ciphertext): {e}")
          print(f"Ciphertext hex: {ciphertext_hex}")
          continue

        try:
          iv = binascii.unhexlify(iv_hex)
        except binascii.Error as e:
          print(f"Error in hex conversion (IV): {e}")
          print(f"IV hex: {iv_hex}")
          continue

        # For AES-CTR in PyCryptodome, if the entire IV is used as the counter block,
        # you can pass an empty nonce and use the IV as the initial counter value.
        initial_value = int.from_bytes(iv, byteorder='big')
        cipher = AES.new(KEY, AES.MODE_CTR, nonce=b'', initial_value=initial_value)
        decrypted_bytes = cipher.decrypt(ciphertext)

        decrypted_bytes = decrypted_bytes.rstrip(b'\x00')  # Strip the null byte padding

        print(decrypted_bytes)

        decrypted_bytes = fix_endianness(decrypted_bytes=decrypted_bytes)
        decrypted_text = decrypted_bytes.decode('utf-8', errors='replace').strip()

        print("Expected plaintext:", expected_plaintext)
        print("Decrypted text  :", decrypted_text)
        if expected_plaintext.strip() == decrypted_text.strip():
          print("Decryption successful!")
        else:
          print("Decryption FAILED!")
        print("-" * 60)
        print("\n\n")
    except Exception as e:
      print("Error:", e)

if __name__ == '__main__':
  main()
