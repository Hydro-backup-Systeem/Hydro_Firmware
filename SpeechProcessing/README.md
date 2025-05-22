Certainly! Here's the `README.md` content wrapped in a `text` block to preserve formatting:

# WAV to C Array Converter

This script converts 16-bit PCM WAV files into C `uint16_t` arrays, ideal for embedding short sound effects in microcontroller firmware. It also provides an option to estimate the flash usage of the generated `.h` files.

## 📁 Project Structure

```text
- AudioFiles     # Place to store/convert audio files
- AudioHeaders   # Place to store generated audio header files
```

> ℹ️ Note: WAV and .h files are excluded from version control due to size.

---

## 🛠 Usage

### Convert a Single WAV File

```sh
python wav_2_array.py --wav path/to/file.wav -o output_file.h -n array_name
````

* `--wav`: Path to the input WAV file.
* `-o`, `--output`: Path to the output `.h` file (defaults to `<stem>.h`)
* `-n`, `--name`: Optional name for the generated C array

### Convert All WAV Files in a Directory

```sh
python wav_2_array.py --dir path/to/wav_dir -o path/to/output_dir
```

* Converts all `.wav` files in the given directory to `.h` files
* `-o`, `--output`: Optional output directory (defaults to the same directory as the input)

### Estimate Flash Usage from Generated Headers

```sh
python wav_2_array.py --estimate path/to/header_dir
```

* Estimates total flash usage of all `.h` files in the directory
* Prints sample counts and byte usage for each file

---

## ⚠️ Requirements

* Python 3.6+
* `numpy` installed (`pip install numpy`)
* `ffmpeg` for mp3 to wav file conversion

---

## 🔊 Notes

* Only 16-bit PCM WAV files are supported.
* For stereo files, only the left channel is used.
* Data is converted to `uint16_t` by shifting from signed to unsigned range.
