import pathlib
import numpy as np
import seaborn as sns

from IPython import display

import tensorflow.compat.v2 as tf
from tensorflow.compat.v2 import keras
from keras.optimizers import Adam
from keras import models
from keras import layers
import matplotlib.pyplot as plt

# Set the seed value for experiment reproducibility.
seed = 327
np.random.seed(seed)
tf.random.set_seed(seed)

# Initialize the data directory
data_dir = pathlib.Path('data/mini_speech_commands')

train_ds, val_ds = keras.utils.audio_dataset_from_directory(
  directory=data_dir,
  batch_size=64,
  validation_split=0.2,
  output_sequence_length=16000,
  seed=0,
  subset='both')

label_names = np.array(train_ds.class_names)

# Change to mono audio
def squeeze(audio, labels):
  audio = tf.squeeze(audio, axis=-1)
  return audio, labels

train_ds = train_ds.map(squeeze, tf.data.AUTOTUNE)
val_ds = val_ds.map(squeeze, tf.data.AUTOTUNE)
test_ds = val_ds.shard(num_shards=2, index=0)
val_ds = val_ds.shard(num_shards=2, index=1)

SAMPLE_FREQUENCY = 16000
FFT_SIZE = 512
STFFT_FRAMES = 256

def get_spectrogram(waveform):
  # Compute STFT
  spectrogram = tf.signal.stft(waveform, frame_length=255, frame_step=128)
  
  # Get magnitude
  spectrogram = tf.abs(spectrogram)
  
  # Convert to log scale (dB)
  spectrogram = 20 * tf.math.log(tf.maximum(spectrogram, 1e-6))
  
  # Add a channels dimension
  spectrogram = spectrogram[..., tf.newaxis]
  
  return spectrogram

def plot_spectrogram(spectrogram, ax):
  if len(spectrogram.shape) > 2:
    assert len(spectrogram.shape) == 3
    spectrogram = np.squeeze(spectrogram, axis=-1)

  # Convert the frequencies to log scale and transpose, so that the time is
  # represented on the x-axis (columns).
  # Add an epsilon to avoid taking a log of zero.
  log_spec = np.log(spectrogram.T + np.finfo(float).eps)
  height = log_spec.shape[0]
  width = log_spec.shape[1]
  X = np.linspace(0, np.size(spectrogram), num=width, dtype=int)
  Y = range(height)
  ax.pcolormesh(X, Y, log_spec)

def make_spec_ds(ds):
  return ds.map(
    map_func=lambda audio,label: (get_spectrogram(audio), label),
    num_parallel_calls=tf.data.AUTOTUNE)

train_spectrogram_ds = make_spec_ds(train_ds)
val_spectrogram_ds = make_spec_ds(val_ds)
test_spectrogram_ds = make_spec_ds(test_ds)

for example_spectrograms, example_spect_labels in train_spectrogram_ds.take(1):
  break

train_spectrogram_ds = train_spectrogram_ds.cache().shuffle(10000).prefetch(tf.data.AUTOTUNE)

val_spectrogram_ds = val_spectrogram_ds.cache().prefetch(tf.data.AUTOTUNE)
test_spectrogram_ds = test_spectrogram_ds.cache().prefetch(tf.data.AUTOTUNE)

input_shape = example_spectrograms.shape[1:]
num_labels = len(label_names)

model = models.Sequential([
  layers.Input(shape=input_shape),

  layers.Conv2D(16, 3, activation=tf.nn.relu),
  layers.MaxPooling2D(),

  layers.Conv2D(32, 3, activation=tf.nn.relu),
  layers.MaxPooling2D(),

  layers.Conv2D(64, 3, activation=tf.nn.relu), 
  layers.MaxPooling2D(),

  layers.GlobalAveragePooling2D(),
  layers.Dense(16, activation=tf.nn.relu),
  layers.Dense(num_labels, activation='softmax'),
])

model.summary()

model.compile(
  optimizer=tf.keras.optimizers.Adam(learning_rate=0.0005),
  loss=tf.keras.losses.SparseCategoricalCrossentropy(),
  metrics=['accuracy'],
)

EPOCHS = 250
history = model.fit(
  train_spectrogram_ds,
  validation_data=val_spectrogram_ds,
  epochs=EPOCHS,
  callbacks=tf.keras.callbacks.EarlyStopping(verbose=1, patience=10, restore_best_weights=True),
)

# Save the model in the correct format
model.save("./models/test.keras")

# Convert to TensorFlow Lite format
# Load the Keras model from the saved file
keras_model = tf.keras.models.load_model("./models/test.keras")

# Function to generate representative dataset (required for quantization)
def representative_dataset_gen():
  for example_spectrograms, example_spect_labels in train_spectrogram_ds.take(100):
    # Yielding example spectrograms, converting them to float32
    yield [tf.cast(example_spectrograms, tf.float32)]

# Convert to TensorFlow Lite format with quantization
converter = tf.lite.TFLiteConverter.from_keras_model(keras_model)

# Set the optimization flag to enable quantization
converter.optimizations = [tf.lite.Optimize.DEFAULT]

# Provide a representative dataset for calibration during quantization
converter.representative_dataset = representative_dataset_gen

# Ensure that the output is quantized to uint8
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.uint8  # Quantize input to int8
converter.inference_output_type = tf.uint8  # Quantize output to int8

# Perform the conversion
tflite_model = converter.convert()

# Save the TFLite model to a file
with open("./models/model_quantized.tflite", "wb") as f:
  f.write(tflite_model)

print("Quantized model has been converted to TFLite format and saved as 'model_quantized.tflite'")
