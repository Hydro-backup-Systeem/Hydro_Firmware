import serial
import numpy as np
import wave
import struct
import matplotlib.pyplot as plt
from scipy.signal import spectrogram

# Serial port settings
SERIAL_PORT = "/dev/ttyACM0"  # Adjust if needed
BAUD_RATE = 115200
FFT_SIZE = 512
NUM_FRAMES = 30  # Number of frames for the spectrogram
NUM_BINS = 128  # Frequency bins per spectrogram frame
BUFFER_SIZE = NUM_FRAMES * FFT_SIZE  # Match STM32 buffer size
CHUNK_SIZE = 256 * 2  # 256 samples * 2 bytes each
FRAME_HEADER = b'\xAA\x55'  # Common 2-byte frame header

# Open Serial Port
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=5)

### STEP 1: READ RAW ADC AUDIO DATA ###
print("Waiting for ADC data...")
while True:
    header = ser.read(2)
    if header == FRAME_HEADER:
        break

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

# Compute spectrogram from ADC data
fs = 16000  # Adjust based on your timer settings
f, t, Sxx = spectrogram(adc_data, fs, nperseg=FFT_SIZE, noverlap=FFT_SIZE//2)

# Convert the spectrogram intensity to dB scale
Sxx_db = 10 * np.log10(Sxx)

# Quantize the calculated spectrogram
def quantize_spectrogram(input_data, min_val=-20.0, max_val=0.0):
    """
    Quantizes the input spectrogram data into uint8 values between 0 and 255,
    assuming the data is in dB scale, with a specified minimum and maximum value.
    """
    # Normalize input data (log scale) to the range [0, 255]
    input_data = np.clip(input_data, min_val, max_val)  # Ensure within min-max bounds
    quantized_data = 255 * (input_data - min_val) / (max_val - min_val)
    return quantized_data.astype(np.uint8)

# Quantize the spectrogram (matching STM32 quantization range)
quantized_spectrogram = quantize_spectrogram(Sxx_db, min_val=-20.0, max_val=0.0)

### STEP 2: READ SPECTROGRAM DATA FROM STM32 ###
print("Waiting for Spectrogram data...")
spectrogram_data = np.zeros((NUM_FRAMES, NUM_BINS))

def read_spectrogram_frame():
    while True:
        header = ser.read(2)
        if header == FRAME_HEADER:
            data = ser.read(NUM_BINS)
            if len(data) == NUM_BINS:
                return np.frombuffer(data, dtype=np.uint8)
            else:
                print("Warning: Incomplete spectrogram frame received")
        else:
            print("Warning: Incorrect header, resyncing...")

for i in range(NUM_FRAMES):
    spectrogram_data[i, :] = read_spectrogram_frame()
    print(f"Received spectrogram frame {i+1}/{NUM_FRAMES}")

spectrogram_data = spectrogram_data.T  # Transpose for visualization

### STEP 3: PLOT THE RESULTS SIDE BY SIDE ###
fig, axs = plt.subplots(1, 2, figsize=(12, 6))

# Plot the quantized computed spectrogram from ADC data
pcm = axs[0].imshow(quantized_spectrogram, aspect='auto', origin='lower', extent=[0, len(t), 0, fs // 2], cmap='viridis')
axs[0].set_ylabel("Frequency (Hz)")
axs[0].set_xlabel("Time (s)")
axs[0].set_title("Quantized Spectrogram (Calculated from ADC Data)")
fig.colorbar(pcm, ax=axs[0], label="Intensity")  # Corrected colorbar

# Plot the spectrogram received from STM32
img = axs[1].imshow(spectrogram_data, aspect='auto', origin='lower', extent=[0, NUM_FRAMES, 0, 8000], cmap='viridis')
axs[1].set_xlabel("Time Frames")
axs[1].set_ylabel("Frequency Bins")
axs[1].set_title("Live Spectrogram (STM32 Data)")
fig.colorbar(img, ax=axs[1], label="Intensity")  # Corrected colorbar

plt.tight_layout()
plt.show()

# Close serial port
ser.close()
