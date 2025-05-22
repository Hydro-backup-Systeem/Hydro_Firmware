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

#include <CleanTask.h>
#include <UpdateTask.h>
#include <EspeakTask.h>
#include <SX1276_IRQ_Handle_Task.h>

#include "sai.h"

#include "Audio/flag.h"
#include "Audio/black.h"
#include "Audio/blue.h"
#include "Audio/green.h"
#include "Audio/meatball.h"
#include "Audio/red.h"
#include "Audio/yellow.h"
#include "Audio/given_flag.h"
#include "Audio/radio_receive.h"

#include "Audio/try_harder.h"
#include "Audio/box-box.h"
#include "Audio/pole-pos.h"

SharedContext_t ctx;

static LoRa lora;
static PacketHandler packetHandler(&lora);

SemaphoreHandle_t uart_mutex;

enum class FlagTypes {
  Black,
  Blue,
  Green,
  Meatball,
  Red,
  Yellow,
};

void schedule_tasks(void) {
  DEBUG_MUTEX = xSemaphoreCreateBinary();

  ctx.sx1276_irq = xEventGroupCreate();
  ctx.flagQueue = xQueueCreate(10, sizeof(uint8_t));
  ctx.presetQueue = xQueueCreate(10, sizeof(uint8_t));
  ctx.synthQueue = xQueueCreate(10, sizeof(array_t<std::shared_ptr<uint8_t[]>>));

  ctx.packetHandler = &packetHandler;
  packetHandler.init();
  ctx.packetHandler->set_msg_callback([](std::shared_ptr<uint8_t[]> data, size_t len) {
    array_t<std::shared_ptr<uint8_t[]>> d = {
      data,
      len
    };

    xQueueSend(ctx.synthQueue, &d, portMAX_DELAY);
  });

  ctx.lora = &lora;
  lora.init();
  lora.set_long_range();


#ifdef OK
  // Debuging without having RiPi
  xTaskCreate([](void* pvParameters) {
    auto* ctx = static_cast<SharedContext_t*>(pvParameters);
    static int i = 0;

    while (true) {
      vTaskDelay(pdMS_TO_TICKS(10000));

      // Hardcoded data for testing
      const char* testData = "you are given";
      // Create a new packet and populate it with hardcoded data
      packet_t* pkt = new packet_t;
      pkt->type = static_cast<uint8_t>(PacketTypes::FLAGS);  // Set the type (you can adjust this)

      uint8_t id = rand();
      while (ctx->packetHandler->received[id] != nullptr) {
        id = rand();
        vTaskDelay(pdMS_TO_TICKS(1));
      }

      pkt->message_id = rand();  // Use random message ID for testing
      pkt->fragment_id = 0;      // Simulating first fragment
      pkt->total_fragments = 1;  // Simulating 1 fragment
      pkt->lenght = 1;
      pkt->data[0] = (uint8_t)rand() % 6;
      pkt->checksum = ctx->packetHandler->compute_checksum(pkt, pkt->lenght);

      message_bucket* bucket = new message_bucket;
      bucket->expiration = 0;
      bucket->frags.push_back(pkt);

      ctx->packetHandler->received[pkt->message_id] = bucket;

      {
        // Hardcoded data for testin
        char testData[PACKET_MAX_SIZE];
        sprintf(testData, "second %d", i++);


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
        memcpy(pkt->data, testData, strlen(testData));
        pkt->checksum = ctx->packetHandler->compute_checksum(pkt, pkt->lenght);

        message_bucket* bucket = new message_bucket;
        bucket->expiration = 0;
        bucket->frags.push_back(pkt);

        ctx->packetHandler->received[pkt->message_id] = bucket;
      }

      BSP_LED_Toggle(LED_BLUE);
    }
  }, "DebugRX_t", 256, &ctx, PRIORITY_DEFAULT, NULL);
#endif

  xTaskCreate([](void* pvParameters) {
    auto* ctx = static_cast<SharedContext_t*>(pvParameters);
    static int i = 0;

    while (true) {
      char buff[128];

      sprintf(buff, "Race car ping: %d\n", i++);

      ctx->packetHandler->send((uint8_t*)buff, strlen(buff), PacketTypes::MSG);

      vTaskDelay(pdMS_TO_TICKS(3500));
    }
  }, "TestTx_t", 256, &ctx, PRIORITY_HIGH, NULL);

  enum class FlagTypes {
    Black,
    Blue,
    Green,
    Meatball,
    Red,
    Yellow,
  };

  xTaskCreate([](void* pvParameters) {
    auto* ctx = static_cast<SharedContext_t*>(pvParameters);

    uint8_t flag_type;

    while (true) {
      if (xQueueReceive(ctx->flagQueue, &flag_type, portMAX_DELAY)) {
        HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(radio_receive), radio_receive_len, HAL_MAX_DELAY);
        HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(given_flag), given_flag_len, HAL_MAX_DELAY);

        switch ((FlagTypes)flag_type) {
          case FlagTypes::Black: {
            HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(black), black_len, HAL_MAX_DELAY);
            break;
          }

          case FlagTypes::Blue: {
            HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(blue), blue_len, HAL_MAX_DELAY);
            break;
          }

          case FlagTypes::Green: {
            HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(green), green_len, HAL_MAX_DELAY);
            break;
          }

          case FlagTypes::Meatball: {
            HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(meatball), meatball_len, HAL_MAX_DELAY);
            break;
          }

          case FlagTypes::Red: {
            HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(red), red_len, HAL_MAX_DELAY);
            break;
          }

          case FlagTypes::Yellow: {
            HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(yellow), yellow_len, HAL_MAX_DELAY);
            break;
          }

          default:
            break;
        }

        HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(flag), flag_len, HAL_MAX_DELAY);
      }
    }
  }, "FlagProcessing_t", 256, &ctx, PRIORITY_HIGHEST, NULL);


  enum class Presets {
    BOX = 1,
    TRYHARDER,
    POLEPOS,
  };

  xTaskCreate([](void* pvParameters) {
    auto* ctx = static_cast<SharedContext_t*>(pvParameters);

    uint8_t preset;

    while (true) {
      if (xQueueReceive(ctx->presetQueue, &preset, portMAX_DELAY)) {
        HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(radio_receive), radio_receive_len, HAL_MAX_DELAY);

        switch((Presets)preset) {
          case Presets::BOX: {HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(box_box), box_box_len, HAL_MAX_DELAY); break;}
          case Presets::POLEPOS: {HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(pole_pos), pole_pos_len, HAL_MAX_DELAY); break;}
          case Presets::TRYHARDER: {HAL_SAI_Transmit(&hsai_BlockA1, (uint8_t *)(try_harder), try_harder_len, HAL_MAX_DELAY); break;}
          default: break;
        }

      }
    }
  }, "PresetProcessing_t", 256, &ctx, PRIORITY_HIGHEST, NULL);

  xTaskCreate(
    SX1276_IRQ_Handle_Task,
    SX1276_IRQ_Handle_TASK_NAME,
    128,
    &ctx,
    PRIORITY_HIGHEST + 1,
    NULL);

  xTaskCreate(
    UpdateTask,
    UPDATE_TASK_NAME,
    128,
    &ctx,
    PRIORITY_DEFAULT,
    &ctx.UpdateTaskHandle);

  xTaskCreate(
    CleanTask,
    Clean_TASK_NAME,
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

  // Begin sensor reading tasks

  // End sensor reading tasks
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    // You can log the task name or handle the error in another way
    DEBUG_STR_LN("Stack Overflow in task: %s\n", pcTaskName);
    taskDISABLE_INTERRUPTS();  // Disable interrupts to prevent further issues

    __asm volatile ("bkpt ""0"); // Breakpoint in debug mode

    for (;;);  // Enter infinite loop to halt the system
}

