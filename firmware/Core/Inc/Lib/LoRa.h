#pragma once

#include "lora_sx1276.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

enum class LoRaMode {
    Receive,
    Transmit
};

class LoRa {
  public:
    LoRa();

  public:
    void init();
    bool set_long_range();
    bool set_short_range();

  public:
    bool send(const uint8_t* data, uint16_t len);
    uint8_t receive(uint8_t* buffer, uint16_t maxLen, uint16_t timeoutMs, uint8_t* errorOut);

  public:
    LoRaMode current_mode() { return mode; }

  private:
    void set_receive_mode();
    void set_transmit_mode();

  private:
    friend void SX1276_IRQ_Handle_Task(void* pvParameters);

  private:
    LoRaMode mode;
    lora_sx1276 lora;

  private:
    SemaphoreHandle_t mtx;

};
