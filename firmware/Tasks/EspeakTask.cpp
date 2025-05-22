/*
 * EspeakTask.cpp
 *
 *  Created on: May 5, 2025
 *      Author: joey
 */

#include "EspeakTask.h"
#include "math_utils.h"

#include "Audio/radio_receive.h"

StaticTask_t  xEspeakTaskTCB;
StackType_t   xEspeakTaskStack[ESPEAK_T_STACK_SIZE];

void EspeakTask(void *pvParameters) {
  auto* ctx = static_cast<SharedContext_t*>(pvParameters);

  array_t<std::shared_ptr<uint8_t[]>> buffer;
// There is something seriously wrong with the speech synthesis
  int result = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 500, NULL, 0);
  configASSERT(result);

  espeak_SetSynthCallback(espeak_callback);

  while (true) {
    if (xQueueReceive(ctx->synthQueue, &buffer, portMAX_DELAY)) {
      HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(radio_receive), radio_receive_len, HAL_MAX_DELAY);

      HAL_UART_Transmit(&huart1, (const uint8_t*)buffer.data.get(), buffer.len, 1000);

      BSP_LED_Toggle(LED_RED);

      espeak_Synth(
          (const char*)buffer.data.get(),
          buffer.len,
          0,
          POS_WORD,
          0,
          espeakCHARS_AUTO | espeakENDPAUSE,
          NULL,
          NULL
      );

      espeak_Synchronize();

      UBaseType_t unused_words = uxTaskGetStackHighWaterMark(NULL);
      UBaseType_t used_bytes = (ESPEAK_T_STACK_SIZE - unused_words) * sizeof(StackType_t);

      DEBUG_INT_LN("Used stack: ", used_bytes);
    }
  }
}

int espeak_callback(short* wav, int numsamples, espeak_EVENT* events) {
  const int window_size = 5;

  while (events->type != 0) {
    if (events->type == espeakEVENT_SENTENCE) {
      BSP_LED_Toggle(LED_RED);  // toggle LED per sentence
    }

    events++;
  }

  if (wav != NULL && numsamples > 0) {
    // Does not  work at the moment
    //    MovingAverageFIR filter(numsamples);
    //    filter.process(wav);

    for (int i = 0; i <= numsamples - window_size; i++) {
      int32_t sum = 0;
      for (int j = 0; j < window_size; j++) {
        sum += wav[i + j];
      }

      wav[i] = (short)(sum / window_size);
    }

    HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(wav), numsamples, HAL_MAX_DELAY);
  }

  return 0;
}
