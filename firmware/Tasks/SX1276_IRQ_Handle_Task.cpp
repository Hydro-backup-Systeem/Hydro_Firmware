#include <SX1276_IRQ_Handle_Task.h>

extern SharedContext_t ctx;

extern "C" {
  void rx_tx_callback() {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    EventBits_t bit = (ctx.lora->current_mode() == LoRaMode::Transmit)
         ? SX1276_TX_DONE
         : SX1276_RX_DONE;

    xEventGroupSetBitsFromISR(ctx.sx1276_irq, bit, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
}

void SX1276_IRQ_Handle_Task(void* pvParameters) {
  auto* ctx = static_cast<SharedContext_t*>(pvParameters);
  auto  eg  = ctx->sx1276_irq;

  while (true) {
    EventBits_t bits = xEventGroupWaitBits(
      eg,
      SX1276_RX_DONE | SX1276_TX_DONE,
      pdTRUE,
      pdFALSE,
      portMAX_DELAY
    );

    if (bits & SX1276_RX_DONE) {
      ctx->packetHandler->receive();

      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      vTaskNotifyGiveFromISR((TaskHandle_t)ctx->UpdateTaskHandle, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

      BSP_LED_Toggle(LED_BLUE);
    }

    if (bits & SX1276_TX_DONE) {
      MutexGuard guard(ctx->lora->mtx, "SX1276_IRQ_Handle_Task", portMAX_DELAY);
      ctx->lora->set_receive_mode();

      BSP_LED_Toggle(LED_GREEN);
    }
  }
}
