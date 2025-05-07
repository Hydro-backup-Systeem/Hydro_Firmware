import serial
import wave
import sys
import subprocess

# === SERIAL CONFIGURATION ===
SERIAL_PORT = '/dev/ttyACM0'  # Update based on your system
BAUDRATE = 576000          
TIMEOUT = None                # Blocking read

# === AUDIO PARAMETERS ===
SAMPLE_RATE = 16000 # Hz (CD Quality)
BITS_PER_SAMPLE = 16  # 16-bit PCM
CHANNELS = 1  # Mono
FRAME_SIZE = 2  # 16-bit = 2 bytes per sample

# === HEADER AND FOOTER SEQUENCES ===
HEADER = bytearray([0xAA, 0x55])
TAIL   = bytearray([0x55, 0xAA])
DONE   = bytearray([0xAA, 0xAA])

# === OPEN SERIAL PORT ===
try:
    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=TIMEOUT)
    print(f"Listening on {SERIAL_PORT} at {BAUDRATE} baud...")
except Exception as e:
    print(f"Error opening serial port: {e}")
    sys.exit(1)

while True:
    audio_data = bytearray()
    payload = bytearray()
    state = 0  # 0: Searching for HEADER, 1: Receiving audio

    try:
        while True:
            byte = ser.read(1)
            if not byte:
                continue

            if state == 0:  # Looking for HEADER
                payload += byte
                if len(payload) > len(HEADER):
                    payload = payload[-len(HEADER):]  # Keep last 2 byte
                if payload == HEADER:
                    payload.clear()
                    state = 1  # Start receiving audio
                elif payload == DONE:
                    # Clear buffer
                    ser.close()  # Close current connection
                    ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=TIMEOUT)  # Reopen serial port
                    break  # Restart loop to listen again


            elif state == 1:  # Receiving audio
                payload += byte

                if len(payload) >= len(TAIL) and payload[-len(TAIL):] == TAIL:
                    chunk = payload[:-len(TAIL)]  # Remove tail
                    audio_data.extend(chunk)
                    payload.clear()
                    state = 0  # Back to searching for HEADER

    except KeyboardInterrupt:
        print("\nTerminating reception.")
        break

    # === SAVE TO WAV ===
    # print(audio_data)
    if len(audio_data) == 0:
        continue

    # Ensure the byte count is even (16-bit alignment)
    if len(audio_data) % 2 != 0:
        audio_data.pop()  # Remove last byte if needed

    print("saving file")
    output_filename = "output.wav"
    try:
        with wave.open(output_filename, 'wb') as wf:
            wf.setnchannels(CHANNELS)
            wf.setsampwidth(BITS_PER_SAMPLE // 8)  # 16-bit = 2 bytes
            wf.setframerate(SAMPLE_RATE)
            wf.writeframes(audio_data)

        print(f"Audio saved to {output_filename}")

        # === PLAYBACK ===
        subprocess.run(["play", "./output.wav"])
        

    except Exception as e:
        print(f"Error writing or playing WAV file: {e}")

# Close serial port when exiting
ser.close()
