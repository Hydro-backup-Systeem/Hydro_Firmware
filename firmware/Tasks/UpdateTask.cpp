#include "UpdateTask.h"

#include "debug.h"

void UpdateTask(void* pvParameters) {
  auto* ctx = static_cast<SharedContext_t*>(pvParameters);

  while (true) {
    ctx->packetHandler->poll();
    ctx->packetHandler->clean();

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
