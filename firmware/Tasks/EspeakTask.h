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
  T data;
  size_t len;
};

#define ESPEAK_TASK_NAME "Espeak_t"

// 48 kBytes of stack memory
#define ESPEAK_T_STACK_SIZE (48 * 1024 / sizeof(StackType_t))

extern StaticTask_t  xEspeakTaskTCB;
extern StackType_t   xEspeakTaskStack[ESPEAK_T_STACK_SIZE];

void EspeakTask(void *pvParameters);

int espeak_callback(short* wav, int numsamples, espeak_EVENT* events);

#endif /* ESPEAKTASK_H_ */
