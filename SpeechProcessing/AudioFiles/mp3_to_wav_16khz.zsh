#!/bin/bash

# Converts all .mp3 files in the current directory and its subdirectories to .wav format at 16 kHz.
# Converted .wav files will be saved in the 'Converted' folder, preserving the directory structure.

# Create the 'Converted' directory if it doesn't exist
mkdir -p Converted

# Function to process mp3 files
convert_mp3_to_wav() {
  local input_file="$1"
  local output_dir="$2"
  
  # Get the relative path of the mp3 file (excluding the initial ./)
  local relative_path="${input_file#./}"
  
  # Replace .mp3 with .wav
  local output_file="${relative_path%.mp3}.wav"
  
  # Create the necessary directory structure in the 'Converted' folder
  mkdir -p "$output_dir/$(dirname "$output_file")"

  # Convert using ffmpeg
  ffmpeg -i "$input_file" -ar 16000 "$output_dir/$output_file"
}

# Export function to use it with find
export -f convert_mp3_to_wav

# Find all .mp3 files recursively and process them
find . -type f -name "*.mp3" -exec bash -c 'convert_mp3_to_wav "$0" "Converted"' {} \;

echo "Conversion complete."
