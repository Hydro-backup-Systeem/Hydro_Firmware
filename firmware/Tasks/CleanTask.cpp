#include "CleanTask.h"

void CleanTask(void *pvParameters) {
  auto* ctx = static_cast<SharedContext_t*>(pvParameters);

  while (true) {
    ctx->packetHandler->clean();

    // Clean every 10 seconds
    vTaskDelay(pdMS_TO_TICKS(10000));
  }
}
