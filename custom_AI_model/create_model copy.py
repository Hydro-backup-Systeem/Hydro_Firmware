import pathlib
import seaborn as sns
import numpy as np
import tensorflow as tf
from tensorflow.keras import layers
from tensorflow.keras import models
import matplotlib.pyplot as plt

from IPython import display

# Set the seed value for experiment reproducibility.
seed = 42
tf.random.set_seed(seed)
np.random.seed(seed)

DATASET_PATH = 'data/mini_speech_commands'

data_dir = pathlib.Path(DATASET_PATH)
if not data_dir.exists():
  tf.keras.utils.get_file(
      'mini_speech_commands.zip',
      origin="http://storage.googleapis.com/download.tensorflow.org/data/mini_speech_commands.zip",
      extract=True,
      cache_dir='.', cache_subdir='data')


train_ds, val_ds = tf.keras.utils.audio_dataset_from_directory(
  directory=data_dir,
  batch_size=64,
  validation_split=0.2,
  output_sequence_length=16000,
  seed=0,
  subset='both')

label_names = np.array(train_ds.class_names)

print(label_names)

# Change to mono audio
def squeeze(audio, labels):
  audio = tf.squeeze(audio, axis=-1)
  return audio, labels

train_ds = train_ds.map(squeeze, tf.data.AUTOTUNE)
val_ds = val_ds.map(squeeze, tf.data.AUTOTUNE)
test_ds = val_ds.shard(num_shards=2, index=0)
val_ds = val_ds.shard(num_shards=2, index=1)

# Creates spectrogram of shape (124, 129, 1)
def get_spectrogram(waveform):
  # Convert the waveform to a spectrogram via a STFT.
  spectrogram = tf.signal.stft(
      waveform, frame_length=255, frame_step=128)
  # Obtain the magnitude of the STFT.
  spectrogram = tf.abs(spectrogram)

  # Add a `channels` dimension, so that the spectrogram can be used
  # as image-like input data with convolution layers (which expect
  # shape (`batch_size`, `height`, `width`, `channels`).
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

for example_audio, example_labels in train_ds.take(1):  
  print(example_audio.shape)
  print(example_labels.shape)

train_spectrogram_ds = make_spec_ds(train_ds)
val_spectrogram_ds = make_spec_ds(val_ds)
test_spectrogram_ds = make_spec_ds(test_ds)
for i in range(3):
  label = label_names[example_labels[i]]
  waveform = example_audio[i]
  spectrogram = get_spectrogram(waveform)

  print('Label:', label)
  print('Waveform shape:', waveform.shape)
  print('Spectrogram shape:', spectrogram.shape)
  print('Audio playback')
  display.display(display.Audio(waveform, rate=16000))
  fig, axes = plt.subplots(2, figsize=(12, 8))
  timescale = np.arange(waveform.shape[0])
  axes[0].plot(timescale, waveform.numpy())
  axes[0].set_title('Waveform')
  axes[0].set_xlim([0, 16000])

  plot_spectrogram(spectrogram.numpy(), axes[1])
  axes[1].set_title('Spectrogram')
  plt.suptitle(label.title())
  plt.show()

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
  layers.Dropout(0.4), 
  layers.Dense(num_labels, activation='softmax'),
])

model.summary()

model.compile(
  optimizer=tf.keras.optimizers.Adam(learning_rate=0.0005),
  loss=tf.keras.losses.SparseCategoricalCrossentropy(),
  metrics=['accuracy'],
)

EPOCHS = 2
history = model.fit(
  train_spectrogram_ds,
  validation_data=val_spectrogram_ds,
  epochs=EPOCHS,
  callbacks=tf.keras.callbacks.EarlyStopping(verbose=1, patience=10, restore_best_weights=True),
)

# Save the model in the correct format
model.save("./models/test.h5")

# Convert to TensorFlow Lite format
# Load the Keras model from the saved file
keras_model = tf.keras.models.load_model("./models/test.h5")

# Function to generate representative dataset (required for quantization)
def representative_dataset_gen():
  for example_spectrograms, example_spect_labels in train_spectrogram_ds.take(100):
    yield [example_spectrograms]

# Convert to TensorFlow Lite format with quantization
converter = tf.lite.TFLiteConverter.from_keras_model(keras_model)

# Set the optimization flag to enable quantization
converter.optimizations = [tf.lite.Optimize.DEFAULT]

# Provide a representative dataset for calibration during quantization
converter.representative_dataset = representative_dataset_gen

# Ensure that the output is quantized to uint8
converter.target_spec.supported_ops = [tf.lite.OpsSet.TFLITE_BUILTINS_INT8]
converter.inference_input_type = tf.int8  # Quantize input to uint8
converter.inference_output_type = tf.int8  # Quantize output to uint8

# Perform the conversion
tflite_model = converter.convert()

# Save the TFLite model to a file
with open("./models/model_quantized.tflite", "wb") as f:
    f.write(tflite_model)

print("Quantized model has been converted to TFLite format and saved as 'model_quantized.tflite'")
