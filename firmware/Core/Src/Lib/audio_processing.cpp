#include "audio_processing.h"

AudioProcessor::AudioProcessor() {
  arm_rfft_fast_init_f32(&fftInstance, FFT_SIZE);
  generate_hann_window();
}

void AudioProcessor::process(const uint16_t* adc_input, uint8_t* spectrogram_output) {
  for (uint8_t i = 0; i < STFFT_FRAMES; i++) {
    uint16_t start_idx = i * FRAME_STEP;

    process_audio_frame(&adc_input[start_idx], &spectrogram_output[i * (NUM_BINS)]);
  }
}

// Generate the Hann window coefficients.
void AudioProcessor::generate_hann_window() {
  for (int i = 0; i < FFT_SIZE; i++) {
    hannWindow[i] = 0.5f * (1.0f - arm_cos_f32(2.0f * PI * i / (FFT_SIZE - 1)));
  }
}

// Apply the precomputed Hann window to data.
void AudioProcessor::apply_hann_window(float* data) {
  for (int i = 0; i < FFT_SIZE; i++) {
    data[i] *= hannWindow[i];
  }
}

// Normalize PCM data from 16-bit ADC values to floats in the range [-1, 1].
void AudioProcessor::normalize_pcm(const uint16_t* input, float* output, uint16_t length) {
  for (int i = 0; i < length; i++) {
    output[i] = (((float)(input[i] & 0x3FFF) / ADC_MAX_VALUE) * 2.0f) - 1.0f;
  }
}

// Compute the real FFT.
void AudioProcessor::compute_fft(const float* input, float* output) {
  // Convert real input to complex format (interleaved real and imaginary parts).
  for (int i = 0; i < FFT_SIZE; i++) {
    fftComplexBuffer[2 * i]   = input[i];   // Real part
    fftComplexBuffer[2 * i + 1] = 0.0f;     // Imaginary part
  }

  // Compute FFT in-place.
  arm_rfft_fast_f32(&fftInstance, fftComplexBuffer, fftComplexBuffer, 0);

  // Compute magnitude: sqrt(Re^2 + Im^2)
  arm_cmplx_mag_f32(fftComplexBuffer, output, (FFT_SIZE / 2) + 1);
}

// Convert the FFT output to a logarithmic (dB) scale.
void AudioProcessor::log_scale_spectrum(float32_t* input, float32_t* output) {
  for (int i = 0; i < NUM_BINS; i++) {
    float value = input[i] + 1e-6f;  // Avoid log(0)
    output[i] = 20.0f * log10f(value);
  }
}

// Quantize the spectrogram data to uint8_t.
void AudioProcessor::quantize_spectrogram(const float32_t* input, uint8_t* output, int size) {
  // Define a minimum dB value.
  float min_val = -20.0f;

  // Find the maximum value in the log spectrum.
  float max_val = min_val;
  for (int i = 0; i < size; i++) {
    if (input[i] > max_val) {
      max_val = input[i];
    }
  }

  // Map input values from [min_val, max_val] to [0, 255]
  for (int i = 0; i < size; i++) {
    output[i] = static_cast<uint8_t>(255.0f * (input[i] - min_val) / (max_val - min_val));
  }
}

// Process an audio frame by chaining normalization, windowing, FFT, log scaling, and quantization.
void AudioProcessor::process_audio_frame(const uint16_t* adc_input, uint8_t* spectrogram_output) {
  // 1. Normalize PCM data.
  normalize_pcm(adc_input, adcFloatBuffer, FFT_SIZE);

  // 2. Apply Hann window.
  apply_hann_window(adcFloatBuffer);

  // 3. Compute FFT.
  compute_fft(adcFloatBuffer, fftMagnitudeBuffer);

  // 4. Convert FFT output to logarithmic (dB) scale.
  log_scale_spectrum(fftMagnitudeBuffer, fftMagnitudeBuffer);

  // 5. Quantize and store the spectrogram.
  quantize_spectrogram(fftMagnitudeBuffer, spectrogram_output, NUM_BINS);
}
