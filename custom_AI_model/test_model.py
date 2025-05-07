import wave
import struct
import tensorflow as tf
import numpy as np
import matplotlib.pyplot as plt
from pvrecorder import PvRecorder
from tensorflow import lite

# Load the TFLite model
interpreter = lite.Interpreter(model_path="./models/model_quantized.tflite")
interpreter.allocate_tensors()

# Input and output details
input_details = interpreter.get_input_details()
output_details = interpreter.get_output_details()

# Function to generate spectrogram
def get_spectrogram(waveform):
    # Convert the waveform to a spectrogram via a STFT.
    spectrogram = tf.signal.stft(waveform, frame_length=255, frame_step=128, fft_length=32)
    # Obtain the magnitude of the STFT.
    spectrogram = tf.abs(spectrogram)

    # Add a `channels` dimension so that the spectrogram can be used
    # as image-like input data with convolution layers (which expect
    # shape (`batch_size`, `height`, `width`, `channels`)).
    spectrogram = spectrogram[..., tf.newaxis]
    return spectrogram

# Quantize the input data to INT8
def quantize_input(spectrogram):
    # Find the min and max values of the spectrogram
    min_value = tf.reduce_min(spectrogram)
    max_value = tf.reduce_max(spectrogram)

    # Scale the spectrogram to fit within the INT8 range [-128, 127]
    spectrogram = 127 * (spectrogram - min_value) / (max_value - min_value) - 128
    spectrogram = tf.round(spectrogram)  # Round to nearest integer

    # Clip the values to make sure they stay within the valid INT8 range
    spectrogram = tf.clip_by_value(spectrogram, -128, 127)
    return tf.cast(spectrogram, tf.int8)

# Test recording and inference
while True:
    recording = []
    recorder = PvRecorder(device_index=-1, frame_length=512)
  
    print(f'Press enter to record')
    input()

    try:
        recorder.start()

        while True:
            frame = recorder.read()
            recording.extend(frame)

    except KeyboardInterrupt:
        recorder.stop()

        print("Creating wave file")
        with wave.open(f'./test.wav', 'w') as f:
            f.setparams((1, 2, 16000, 512, "NONE", "NONE"))
            f.writeframes(struct.pack("h" * len(recording), *recording))

        # Read the saved wave file
        x = tf.io.read_file(f'./test.wav')
        x, sample_rate = tf.audio.decode_wav(x, desired_channels=1, desired_samples=16000)
        x = tf.squeeze(x, axis=-1)  # Remove extra channel dimension if present
        waveform = x

        # Convert the waveform to a spectrogram
        spectrogram = get_spectrogram(waveform)
        spectrogram = spectrogram[tf.newaxis, ...]  # Add batch dimension

        # Quantize the spectrogram to INT8
        quantized_spectrogram = quantize_input(spectrogram)

        # Prepare the input tensor for the TFLite model
        input_data = np.array(quantized_spectrogram, dtype=np.int8)

        # Set the input tensor of the TFLite model
        interpreter.set_tensor(input_details[0]['index'], input_data)

        # Run inference
        interpreter.invoke()

        # Get the output tensor and convert to float32
        output_data = interpreter.get_tensor(output_details[0]['index'])
        output_data = tf.cast(output_data, tf.float32)  # Convert output to float32
        
        print(f'Output data: {output_data}')

        # Apply softmax to the output to get class probabilities
        probabilities = tf.nn.softmax(output_data[0])

        # Define class labels (replace with your actual labels)
        x_labels = ['Bye', 'Hello']  # Update this as needed
        
        # Plot the predictions
        plt.bar(x_labels, probabilities)
        plt.title('Prediction')
        plt.show()

        recorder.delete()  # Clean up the recorder
