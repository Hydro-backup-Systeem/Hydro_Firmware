#pragma once

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "debug.h"

class MutexGuard {

  public:
    MutexGuard(SemaphoreHandle_t mutex, const char* tag = nullptr, TickType_t timeout = pdMS_TO_TICKS(10))
      : acquired(false), mtx(mutex) {

      configASSERT(mtx != nullptr);

      acquired = xSemaphoreTake(mtx, timeout) == pdTRUE;

      if (!acquired && tag != nullptr) {
        DEBUG_STR_LN("Did not acquire lock in: ", tag);
      }
    };

    ~MutexGuard() {
      if (acquired) {
        xSemaphoreGive(mtx);
      }
    }

  public:
    bool ok() const { return acquired; }

  private:
    bool acquired;
    SemaphoreHandle_t mtx;

};


