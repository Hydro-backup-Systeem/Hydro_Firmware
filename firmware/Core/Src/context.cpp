#include <context.h>

#include "main.h"
#include "usart.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

// 3th party library include
#include "debug.h"
#include "packet_handler.h"

// System include
#include <stdlib.h>
#include <string.h>

// Task includes
#include "shared_context.h"

#include <UpdateTask.h>
#include <EspeakTask.h>
#include <SX1276_IRQ_Handle_Task.h>

SharedContext_t ctx;

static LoRa lora;
static PacketHandler packetHandler(&lora);

SemaphoreHandle_t uart_mutex;

void schedule_tasks(void) {
  DEBUG_MUTEX = xSemaphoreCreateBinary();

  ctx.sx1276_irq = xEventGroupCreate();
  ctx.synthQueue = xQueueCreate(10, sizeof(array_t<uint8_t>));

  ctx.packetHandler = &packetHandler;
  packetHandler.init();
  ctx.packetHandler->set_msg_callback([](uint8_t* data, size_t len) {
    array_t<uint8_t> d = {
      data,
      len
    };

    DEBUGLN("");
    HAL_UART_Transmit(&huart1, d.data, len, 1000);
    DEBUGLN("");

    xQueueSend(ctx.synthQueue, &d, portMAX_DELAY);
  });

  ctx.lora = &lora;
  lora.init();
  lora.set_long_range();


#ifdef DEBUG
  // Debuging without having RiPi
  xTaskCreate([](void* pvParameters) {
    auto* ctx = static_cast<SharedContext_t*>(pvParameters);

    while (true) {
      vTaskDelay(pdMS_TO_TICKS(1000));

      // Hardcoded data for testing
      const char* testData = "meow meow im a cat";

      // Create a new packet and populate it with hardcoded data
      packet_t* pkt = new packet_t;
      pkt->type = static_cast<uint8_t>(PacketTypes::MSG);  // Set the type (you can adjust this)

      uint8_t id = rand();
      while (ctx->packetHandler->received[id] != nullptr) {
        id = rand();
        vTaskDelay(pdMS_TO_TICKS(1));
      }

      pkt->message_id = rand();  // Use random message ID for testing
      pkt->fragment_id = 0;      // Simulating first fragment
      pkt->total_fragments = 1;  // Simulating 1 fragment
      pkt->lenght = strlen(testData);
      memcpy(pkt->data, testData, pkt->lenght);
      pkt->checksum = ctx->packetHandler->compute_checksum(pkt, pkt->lenght);

      message_bucket* bucket = new message_bucket;
      bucket->expiration = 0;
      bucket->frags.push_back(pkt);

      ctx->packetHandler->received[pkt->message_id] = bucket;
    }
  }, "DebugRX_t", 256, &ctx, PRIORITY_DEFAULT, NULL);
#endif

  xTaskCreate([](void* pvParameters) {
    auto* ctx = static_cast<SharedContext_t*>(pvParameters);

    while (true) {
      const char* msg = "Oi pizza boi";

      ctx->packetHandler->send((uint8_t*)msg, sizeof(msg), PacketTypes::MSG);
      BSP_LED_Toggle(LED_BLUE);

      vTaskDelay(pdMS_TO_TICKS(11020));
    }
  }, "TestTx_t", 256, &ctx, PRIORITY_HIGH, NULL);

  xTaskCreate(
    SX1276_IRQ_Handle_Task,
    SX1276_IRQ_Handle_TASK_NAME,
    128,
    &ctx,
    PRIORITY_HIGHEST,
    NULL);

  xTaskCreate(
    UpdateTask,
    UPDATE_TASK_NAME,
    128,
    &ctx,
    PRIORITY_DEFAULT,
    NULL);

  xTaskCreateStatic(
    EspeakTask,
    ESPEAK_TASK_NAME,
    sizeof(xEspeakTaskStack)/sizeof(xEspeakTaskStack[0]),
    &ctx,
    PRIORITY_HIGHEST,
    xEspeakTaskStack,
    &xEspeakTaskTCB
  );
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    // You can log the task name or handle the error in another way
    DEBUG_STR_LN("Stack Overflow in task: %s\n", pcTaskName);
    taskDISABLE_INTERRUPTS();  // Disable interrupts to prevent further issues

    __asm volatile ("bkpt ""0"); // Breakpoint in debug mode

    for (;;);  // Enter infinite loop to halt the system
}

