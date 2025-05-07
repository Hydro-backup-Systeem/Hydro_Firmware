#pragma once

#include "stdint.h"
#include "arm_math.h"

class AudioProcessor {
  public:
    static constexpr float ADC_MAX_VALUE = 16383.0f;
    static constexpr int SAMPLE_FREQ      = 16000;
    static constexpr int FRAME_LENGTH     = 255;
    static constexpr int FRAME_STEP       = 128;
    static constexpr int FFT_SIZE         = 256;             // FRAME_LENGTH + zero-pad
    static constexpr int NUM_BINS         = (FFT_SIZE / 2);    // 128 bins
    static constexpr int BUFFER_SIZE      = SAMPLE_FREQ;       // 1-second buffer
    static constexpr int STFFT_FRAMES     = 124;               // 124 frames

  public:
    AudioProcessor();

  public:
    void process(const uint16_t* adc_input, uint8_t* spectrogram_output);

  private:
    void generate_hann_window();
    void apply_hann_window(float* data);
    void normalize_pcm(const uint16_t* input, float* output, uint16_t length);
    void compute_fft(const float* input, float* output);
    void log_scale_spectrum(float32_t* input, float32_t* output);
    void quantize_spectrogram(const float32_t* input, uint8_t* output, int size);
    void process_audio_frame(const uint16_t* adc_input, uint8_t* spectrogram_output);
  
  private:
    // FFT instance and buffers
    arm_rfft_fast_instance_f32 fftInstance;
    
    float adcFloatBuffer[FFT_SIZE];
    float fftMagnitudeBuffer[FFT_SIZE];
    float fftComplexBuffer[FFT_SIZE * 2];
    float hannWindow[FFT_SIZE];
};
