import os
import random
import numpy as np
from pydub import AudioSegment

def add_noise(audio, noise_level=0.02):
    """Adds Gaussian noise to an audio segment."""
    samples = np.array(audio.get_array_of_samples()).astype(np.float32)
    
    # Generate random Gaussian noise
    noise = np.random.normal(0, np.std(samples) * noise_level, samples.shape)
    
    # Add noise and clip to prevent overflow
    noisy_samples = np.clip(samples + noise, -32768, 32767).astype(np.int16)

    # Convert back to an AudioSegment
    noisy_audio = AudioSegment(
        noisy_samples.tobytes(),
        frame_rate=audio.frame_rate,
        sample_width=audio.sample_width,
        channels=audio.channels
    )
    
    return noisy_audio

def resample_and_augment(directory):
    """Resamples all .wav files in the specified directory to 16 kHz and creates a noisy version."""
    for root, _, files in os.walk(directory):
        for filename in files:
            if filename.lower().endswith(".wav"):
                filepath = os.path.join(root, filename)
                noisy_filepath = os.path.join(root, filename.replace(".wav", "_noisy.wav"))
                
                try:
                    # Load the audio file
                    audio = AudioSegment.from_file(filepath, format='wav')
                    
                    # Resample to 16 kHz
                    audio = audio.set_frame_rate(16000)
                    
                    # Export back to the same file (overwrite original)
                    audio.export(filepath, format='wav')
                    print(f"Resampled: {filepath}")

                    # Create and save noisy version
                    noisy_audio = add_noise(audio, noise_level=0.02)
                    noisy_audio.export(noisy_filepath, format='wav')
                    print(f"Created noisy version: {noisy_filepath}")

                except Exception as e:
                    print(f"Error processing {filepath}: {e}")

if __name__ == "__main__":
    directory = "./data/mini_speech_commands/"  # Set your directory path here
    if os.path.isdir(directory):
        resample_and_augment(directory)
    else:
        print("Invalid directory path.")
