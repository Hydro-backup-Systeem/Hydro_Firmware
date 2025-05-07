/*
 * EspeakTask.h
 *
 *  Created on: May 5, 2025
 *      Author: joey
 */

#ifndef ESPEAKTASK_H_
#define ESPEAKTASK_H_

#include "sai.h"

#include "espeak_lib.h"
#include "shared_context.h"

template<typename T>
struct array_t {
  T* data;
  size_t len;
};

#define ESPEAK_TASK_NAME "Espeak_t"

static StaticTask_t  xEspeakTaskTCB;
static StackType_t   xEspeakTaskStack[20 * 1024 / sizeof(StackType_t)];

void EspeakTask(void *pvParameters);

int espeak_callback(short* wav, int numsamples, espeak_EVENT* events);

#endif /* ESPEAKTASK_H_ */
