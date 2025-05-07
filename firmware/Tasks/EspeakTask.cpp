/*
 * EspeakTask.cpp
 *
 *  Created on: May 5, 2025
 *      Author: joey
 */

#include "EspeakTask.h"
#include "math_utils.h"


void EspeakTask(void *pvParameters) {
  auto* ctx = static_cast<SharedContext_t*>(pvParameters);

  array_t<uint8_t> buffer;

  int result = espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 500, NULL, 0);
  configASSERT(result);

  espeak_SetSynthCallback(espeak_callback);

  while (true) {
    if (xQueueReceive(ctx->synthQueue, &buffer, portMAX_DELAY)) {
      espeak_ERROR err = espeak_Synth(
          buffer.data,
          buffer.len,
          0,
          POS_WORD,
          0,
          espeakCHARS_AUTO | espeakENDPAUSE,
          NULL,
          NULL
      );

      DEBUG_UINT_LN("Espeak err: ", err);
    }
  }
}

int espeak_callback(short* wav, int numsamples, espeak_EVENT* events) {
  if (wav != NULL && numsamples > 0) {
    MovingAverageFIR filter(numsamples);
    filter.process(wav);

    uint8_t err = HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(wav), numsamples, HAL_MAX_DELAY);

    BSP_LED_Toggle(LED_RED);
  }

  return 0;
}
