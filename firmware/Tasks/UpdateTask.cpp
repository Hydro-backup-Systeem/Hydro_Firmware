#include "UpdateTask.h"

#include "debug.h"

void UpdateTask(void* pvParameters) {
  auto* ctx = static_cast<SharedContext_t*>(pvParameters);

  while (true) {
    // TODO: LoRa check send packets
    // TODO: packetHandler check if received or not

    ctx->packetHandler->poll();
    ctx->packetHandler->clean();

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
