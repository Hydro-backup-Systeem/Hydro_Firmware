import serial
import numpy as np
import wave
import struct
import matplotlib.pyplot as plt
from scipy.signal import spectrogram

# Serial port settings
SERIAL_PORT = "/dev/ttyACM0" 
BAUD_RATE = 115200
FFT_SIZE = 512
BUFFER_SIZE = 30 * FFT_SIZE  # Match STM32 buffer size
CHUNK_SIZE = 256 * 2  # Number of bytes per chunk (256 samples * 2 bytes each)

# Open Serial Port
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=5)

# Wait for header (0xAA, 0x55)
while True:
    header = ser.read(2)
    if header == b'\xAA\x55':
        break

# Read data in chunks
raw_data = bytearray()
while len(raw_data) < BUFFER_SIZE * 2:
    raw_data.extend(ser.read(CHUNK_SIZE))

# Convert to numpy array (uint16 -> int16)
adc_data = np.frombuffer(raw_data, dtype=np.uint16)
adc_data = adc_data.astype(np.int16) - 2048  # Convert to signed PCM

# Save as WAV file
wav_filename = "recorded_audio.wav"
with wave.open(wav_filename, "w") as wf:
    wf.setnchannels(1)  # Mono
    wf.setsampwidth(2)  # 16-bit audio
    wf.setframerate(16000)  # Adjust sample rate if needed
    wf.writeframes(adc_data.tobytes())

print(f"WAV file saved: {wav_filename}")

# Plot the Spectrogram
fs = 16000  # Adjust based on your timer settings
f, t, Sxx = spectrogram(adc_data, fs, nperseg=FFT_SIZE, noverlap=FFT_SIZE//2)

plt.figure(figsize=(10, 6))
plt.pcolormesh(t, f, 10 * np.log10(Sxx), shading="gouraud")
plt.ylabel("Frequency (Hz)")
plt.xlabel("Time (s)")
plt.colorbar(label="dB")
plt.title("Spectrogram")
plt.show()
