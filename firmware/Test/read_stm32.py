import serial
import numpy as np
import matplotlib.pyplot as plt
from scipy.signal import spectrogram

# ----- Configuration -----
# Adjust these parameters to match your STM32 serial settings
SERIAL_PORT = '/dev/ttyACM0'  # e.g., 'COM3' on Windows or '/dev/ttyUSB0' on Linux
BAUD_RATE = 115200            # Change as needed
TIMEOUT = None                   # Seconds

# STM32 data sizes from your code:
FFT_SIZE = 512
STFFT_FRAMES = 129
BUFFER_SIZE = 16000
ADC_BYTES = BUFFER_SIZE * 2             # Each sample is uint16 (2 bytes)
CHUNK_SIZE = 256                        # (used on STM32 for transmission)

# Spectrogram transmission:
# For each frame, STM32 sends 2 bytes header and 128 bytes of spectrogram data.
NUM_SPEC_FRAMES = STFFT_FRAMES
SPEC_FRAME_HEADER = b'\xAA\x55'
SPEC_FRAME_SIZE = 128  # bytes per frame data

# ----- Open Serial Port -----
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=TIMEOUT)
print(f"Opened serial port: {SERIAL_PORT}")

# ----- Read ADC Data Block -----
print("Reading ADC data...")
adc_data_bytes = ser.read(ADC_BYTES)
if len(adc_data_bytes) != ADC_BYTES:
    print(f"Error: Expected {ADC_BYTES} bytes of ADC data, but received {len(adc_data_bytes)} bytes.")
    ser.close()
    exit(1)

# Convert ADC data (little-endian) to numpy array of uint16
adc_data = np.frombuffer(adc_data_bytes, dtype=np.uint16)
print(f"Received ADC data: {adc_data.shape[0]} samples.")

# ----- Read STM32-Generated Spectrogram Data -----
print("Reading STM32 spectrogram frames...")
spec_frames = []
for frame in range(NUM_SPEC_FRAMES):
    header = ser.read(len(SPEC_FRAME_HEADER))
    if header != SPEC_FRAME_HEADER:
        print(f"Warning: Frame {frame} header mismatch. Received: {header}")
        # Optionally, you might want to continue or break out here.
    frame_data = ser.read(SPEC_FRAME_SIZE)
    if len(frame_data) != SPEC_FRAME_SIZE:
        print(f"Error: Expected {SPEC_FRAME_SIZE} bytes for frame {frame}, got {len(frame_data)} bytes.")
        break
    # Convert the frame data to a numpy array of uint8
    spec_frame = np.frombuffer(frame_data, dtype=np.uint8)
    spec_frames.append(spec_frame)

if len(spec_frames) != NUM_SPEC_FRAMES:
    print(f"Error: Expected {NUM_SPEC_FRAMES} spectrogram frames, got {len(spec_frames)}.")
    ser.close()
    exit(1)

# Stack frames into a 2D array (time x frequency)
# The resulting array will be of shape (STFFT_FRAMES, SPEC_FRAME_SIZE)
spec_received = np.stack(spec_frames, axis=0).T
print("Received spectrogram shape:", spec_received.shape)

# ----- Plot Received Spectrogram -----
plt.figure(figsize=(8, 4))
plt.imshow(spec_received, aspect='auto', origin='lower', cmap='viridis')
plt.title("STM32-Generated Spectrogram")
plt.xlabel("Time Frame")
plt.ylabel("Frequency Bin")
plt.colorbar(label="Intensity")
plt.show()

# ----- Compute Spectrogram from ADC Data -----
# ----- Compute Spectrogram from ADC Data -----
# Assuming a sample frequency of 16 kHz
fs = 16000

# Compute spectrogram using a window size (nperseg) of FFT_SIZE and a 50% overlap
f, t, Sxx = spectrogram(adc_data.astype(np.float32), fs=fs, nperseg=FFT_SIZE, noverlap=FFT_SIZE//2)
# Convert power spectrogram to decibels
Sxx_db = 10 * np.log10(Sxx + 1e-6)

# Normalize the dB values to 0-255
Sxx_min = Sxx_db.min()
Sxx_max = Sxx_db.max()
if Sxx_max - Sxx_min > 0:
    Sxx_scaled = ((Sxx_db - Sxx_min) / (Sxx_max - Sxx_min) * 255).astype(np.uint8)
else:
    Sxx_scaled = np.zeros_like(Sxx_db, dtype=np.uint8)

plt.figure(figsize=(8, 4))
plt.pcolormesh(t, f, Sxx_scaled, shading='gouraud', cmap='viridis')
plt.title("Computed Spectrogram from ADC Data (0-255 Scale)")
plt.ylabel("Frequency [Hz]")
plt.xlabel("Time [sec]")
plt.colorbar(label="Intensity (0-255)")
plt.show()

ser.close()
print("Serial port closed.")
