#include <LoRa.h>

#include "MutexGuard.h"
#include "shared_context.h"

#include "spi.h"
#include "gpio.h"

LoRa::LoRa() : mode(LoRaMode::Receive) { };

void LoRa::init() {
  mtx = xSemaphoreCreateMutex();

  uint8_t res = lora_init(&lora, &hspi3, SPI_NSS_GPIO_Port, SPI_NSS_Pin, LORA_BASE_FREQUENCY_EU);
  configASSERT(res == LORA_OK);

  set_receive_mode();
}

bool LoRa::set_long_range() {
  MutexGuard guard(mtx, "LoRa::set_long_range");
  if (!guard.ok()) {
    return false;
  }

  lora_mode_sleep(&lora);

  lora_set_spreading_factor(&lora, 8);
  lora_set_signal_bandwidth(&lora, LORA_BANDWIDTH_62_5_KHZ);
  lora_set_coding_rate(&lora, LORA_CODING_RATE_4_8);
  lora_set_tx_power(&lora, 20);
  lora_set_preamble_length(&lora, 10);
  lora_set_crc(&lora, 1);

  set_receive_mode();

  return true;
}

bool LoRa::set_short_range() {
  MutexGuard guard(mtx, "LoRa::set_short_range");
  if (!guard.ok()) {
    return false;
  }

  lora_mode_sleep(&lora);

  lora_set_spreading_factor(&lora, 7);
  lora_set_signal_bandwidth(&lora, LORA_BANDWIDTH_125_KHZ);
  lora_set_coding_rate(&lora, LORA_CODING_RATE_4_5);
  lora_set_tx_power(&lora, 10);
  lora_set_preamble_length(&lora, 5);
  lora_set_crc(&lora, 0);

  set_receive_mode();

  return true;
}

bool LoRa::send(const uint8_t* data, uint16_t len) {
  MutexGuard guard(mtx, "LoRa::send");
  if (!guard.ok()) {
    return false;
  }

  uint8_t status = lora_send_packet(&lora, (uint8_t*)data, len);
  bool succes = status == LORA_OK;

  if (succes) {
    set_transmit_mode();
  } else {
    set_receive_mode();
  }

  return succes;
}

uint8_t LoRa::receive(uint8_t* buffer, uint16_t maxLen, uint16_t timeoutMs, uint8_t* errorOut) {
  MutexGuard guard(mtx, "LoRa::receive");
  if (!guard.ok()) {
    return 0;
  }

  return lora_receive_packet_blocking(&lora, buffer, maxLen, timeoutMs, errorOut);
}

void LoRa::set_receive_mode() {
  lora_mode_receive_continuous(&lora);
  lora_enable_interrupt_rx_done(&lora);
  mode = LoRaMode::Receive;
}

void LoRa::set_transmit_mode() {
  lora_enable_interrupt_tx_done(&lora);
  mode = LoRaMode::Transmit;
}
