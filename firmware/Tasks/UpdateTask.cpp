#include "UpdateTask.h"

#include "debug.h"

void UpdateTask(void* pvParameters) {
  auto* ctx = static_cast<SharedContext_t*>(pvParameters);

  while (true) {
    // Wait for notification
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(5000)); // TODO: Change back to portMaxDelay

    ctx->packetHandler->check_and_process();
  }
}
