import serial
import numpy as np
import matplotlib.pyplot as plt

# Serial port configuration
SERIAL_PORT = "/dev/ttyACM0"  # Adjust for your system
BAUD_RATE = 115200
NUM_BINS = 128  # Number of frequency bins per frame
NUM_FRAMES = 30  # Number of frames for the spectrogram
FRAME_HEADER = b'\xAA\x55'  # Updated frame header (2 bytes)

# Initialize serial connection
ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)

# Initialize an empty buffer for the spectrogram
spectrogram_data = np.zeros((NUM_FRAMES, NUM_BINS))

# Function to read a single frame
def read_frame():
    while True:
        # Search for the 2-byte header
        header = ser.read(2)
        if header == FRAME_HEADER:
            # Read the spectrogram data
            data = ser.read(NUM_BINS)
            if len(data) == NUM_BINS:
                return np.frombuffer(data, dtype=np.uint8)
            else:
                print("Warning: Incomplete frame received")
        else:
            print("Warning: Incorrect header, resyncing...")

# Collect spectrogram frames
for i in range(NUM_FRAMES):
    spectrogram_data[i, :] = read_frame()
    print(f"Received frame {i+1}/{NUM_FRAMES}")

# Transpose data to match expected axis orientation
spectrogram_data = spectrogram_data.T

# Display the spectrogram
plt.imshow(spectrogram_data, aspect='auto', origin='lower', extent=[0, NUM_FRAMES, 0, 8000], cmap='viridis')
plt.colorbar(label='Intensity')
plt.xlabel('Time Frames')
plt.ylabel('Frequency Bins')
plt.title('Live Spectrogram')
plt.show()

# Close serial port
ser.close()
