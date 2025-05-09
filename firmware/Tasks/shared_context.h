/*
 * shared_context.h
 *
 *  Created on: Apr 10, 2025
 *      Author: joey
 */

#ifndef INC_SHARED_CONTEXT_H_
#define INC_SHARED_CONTEXT_H_

#include "FreeRTOS.h"
#include "semphr.h"

#include "LoRa.h"
#include "event_groups.h"
#include "packet_handler.h"

#define PRIORITY_HIGHEST    7
#define PRIORITY_HIGH       6
#define PRIORITY_DEFAULT    5
#define PRIORITY_LOW        4
#define PRIORITY_LOWEST     3

constexpr EventBits_t SX1276_RX_DONE = (1 << 0);
constexpr EventBits_t SX1276_TX_DONE = (1 << 1);

typedef struct {
    QueueHandle_t synthQueue;
    EventGroupHandle_t sx1276_irq;

    TaskHandle_t UpdateTaskHandle;

    LoRa* lora;
    PacketHandler* packetHandler;
} SharedContext_t;

#endif /* INC_SHARED_CONTEXT_H_ */
