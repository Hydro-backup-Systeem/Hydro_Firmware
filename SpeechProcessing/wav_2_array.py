import os
import wave
import argparse
import textwrap
import numpy as np
from pathlib import Path

def wav_to_c_array(wav_path, array_name="audio_data"):
  with wave.open(str(wav_path), 'rb') as wf:
    n_channels = wf.getnchannels()
    sampwidth = wf.getsampwidth()
    framerate = wf.getframerate()
    n_frames = wf.getnframes()
    raw_data = wf.readframes(n_frames)
    print(f"Processing '{wav_path}' at {framerate} Hz")

  if sampwidth != 2:
    raise ValueError("Only 16-bit WAV files are supported")

  audio = np.frombuffer(raw_data, dtype=np.int16)
  if n_channels == 2:
    audio = audio[0::2]

  audio_u16 = np.uint16(audio + 32768)

  c_lines = [
    f"// Generated from {wav_path}",
    "#include <stdint.h>",
    "#include <stddef.h>",
    "",
    f"const uint16_t {array_name}[] = {{"
  ]

  line = "  "
  for i, sample in enumerate(audio_u16):
    line += f"{sample}, "
    if (i + 1) % 12 == 0:
      c_lines.append(line)
      line = "  "
  if line.strip():
    c_lines.append(line)
  c_lines.append("};")
  c_lines.append(f"const size_t {array_name}_len = sizeof({array_name}) / sizeof({array_name}[0]);")

  return "\n".join(c_lines)

def process_single_file(wav_path, output_path, array_name):
  c_code = wav_to_c_array(wav_path, array_name)
  with open(output_path, 'w') as f:
    f.write(textwrap.dedent(c_code))
  print(f"Wrote C header to {output_path}")

def estimate_flash_usage(header_dir):
  header_dir = Path(header_dir)
  if not header_dir.is_dir():
    raise ValueError(f"{header_dir} is not a directory")

  total_bytes = 0

  for header_file in header_dir.glob("*.h"):
    with open(header_file, 'r') as f:
      lines = f.readlines()

    array_started = False
    element_count = 0

    for line in lines:
      line = line.strip()

      if line.startswith("const uint16_t") and line.endswith("[] = {"):
        array_started = True
        continue

      if array_started:
        if line == "};":
          break
        nums = [x.strip() for x in line.split(",") if x.strip()]
        element_count += len(nums)

    bytes_used = element_count * 2
    total_bytes += bytes_used
    print(f"{header_file.name}: {element_count} samples → {bytes_used} bytes")

  print(f"\n🔢 Total Flash usage: {total_bytes} bytes ({total_bytes / 1024:.2f} KB)")

def main():
  parser = argparse.ArgumentParser(
    description="Convert 16-bit WAV files to C uint16_t arrays or estimate flash usage."
  )

  mode = parser.add_mutually_exclusive_group(required=True)
  mode.add_argument("--wav", help="Path to a single WAV file to convert")
  mode.add_argument("--dir", help="Directory containing multiple WAV files to convert")
  mode.add_argument("--estimate", help="Directory with .h files to estimate total flash usage")

  parser.add_argument("-o", "--output", help=(
    "For --wav: output .h file path\n"
    "For --dir: output directory for .h files"
  ))
  parser.add_argument("-n", "--name", help="C array name (used only with --wav)")

  args = parser.parse_args()

  if args.estimate:
    estimate_flash_usage(args.estimate)

  elif args.dir:
    wav_dir = Path(args.dir)
    if not wav_dir.is_dir():
      raise ValueError(f"{args.dir} is not a directory")

    output_dir = Path(args.output) if args.output else wav_dir

    for wav_file in wav_dir.glob("*.wav"):
      base_name = wav_file.stem
      output_file = output_dir / f"{base_name}.h"
      c_code = wav_to_c_array(wav_file, array_name=base_name)
      with open(output_file, 'w') as f:
        f.write(textwrap.dedent(c_code))
      print(f"Wrote C header to {output_file}")

  elif args.wav:
    wav_path = Path(args.wav)
    if not wav_path.is_file():
      raise ValueError(f"{args.wav} is not a valid file")

    array_name = args.name if args.name else wav_path.stem
    output_path = Path(args.output) if args.output else Path(f"{array_name}.h")
    process_single_file(wav_path, output_path, array_name)

if __name__ == "__main__":
  main()
