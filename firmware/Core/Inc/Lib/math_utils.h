#pragma once

#include "arm_math.h"
#include "FreeRTOS.h"
#include "task.h"
#include <cstdint>

/**
 * @brief 5-tap moving average FIR filter for Q15 (int16_t) data.
 *
 * This class allocates its state buffer dynamically based on the block size.
 * It uses CMSIS-DSP arm_fir_q15 internally to perform the convolution.
 */
class MovingAverageFIR {
  public:
    static constexpr uint32_t NUM_TAPS = 5;
    // Q15 coefficients for 1/5 average = round(0.2 * 32768)
    static constexpr q15_t COEFF[NUM_TAPS] = { 6554, 6554, 6554, 6554, 6554 };

    /**
     * @brief Construct a moving-average filter for a given block size.
     * @param blockSize Number of samples per processing block.
     */
    explicit MovingAverageFIR(uint32_t blockSize);

    /**
     * @brief Destroy the filter and free allocated state.
     */
    ~MovingAverageFIR();

    /**
     * @brief Process a block of samples in-place.
     * @param wav Pointer to int16_t buffer of length blockSize.
     */
    void process(int16_t* wav);

  private:
    uint32_t             _blockSize;
    arm_fir_instance_q15 _fir;
    q15_t*               _state;
};
