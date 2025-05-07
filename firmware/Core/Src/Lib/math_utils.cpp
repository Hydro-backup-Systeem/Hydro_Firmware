#include "math_utils.h"

#include <arm_math.h>
#include <FreeRTOS.h>
#include <task.h>
#include <cstdlib>

MovingAverageFIR::MovingAverageFIR(uint32_t blockSize)
    : _blockSize(blockSize), _state(nullptr)
{
    uint32_t stateSize = NUM_TAPS + blockSize - 1;
    _state = new q15_t[stateSize];
    configASSERT(_state != nullptr);

    arm_fir_init_q15(&_fir, NUM_TAPS, COEFF, _state, blockSize);
}

MovingAverageFIR::~MovingAverageFIR()
{
    if (_state)
    {
        delete[] _state;
        _state = nullptr;
    }
}

void MovingAverageFIR::process(int16_t* wav)
{
    arm_fir_q15(&_fir, reinterpret_cast<q15_t*>(wav), reinterpret_cast<q15_t*>(wav), _blockSize);
}
