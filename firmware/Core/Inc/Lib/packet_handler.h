/*
 * PacketHandler.h
 *
 *  Created on: Mar 27, 2025
 *    Author: joey
 */

#ifndef INC_PACKET_HANDLER_H_
#define INC_PACKET_HANDLER_H_

#include <array>
#include <vector>
#include <cstdint>
#include <unordered_map>

// Encryption & random number generation
#include "aes.h"
#include "rng.h"

// SPI and LoRa definitions
#include "spi.h"
#include "packet.h"

#include "LoRa.h"
#include "MutexGuard.h"

#include "FreeRTOS.h"
#include "semphr.h"

#define LEASE_TIME_MS 2500

typedef struct message_bucket_t {
  uint32_t expiration;
  std::vector<packet_t*> frags;
} message_bucket;

typedef void(*msg_data_callback_t)(uint8_t* data, size_t len);

class PacketHandler {
  public:
    PacketHandler(LoRa* lora);

  public:
    void init();
    void set_msg_callback(msg_data_callback_t callback);

  public:
    // Regular processing methods.
    void poll();
    void clean();
    void receive();
    void send(uint8_t* data, uint32_t size, PacketTypes type);

  private:
    // Low-level send operation for one packet.
    void send_pkt(packet_t* pkt);

  public:
    // Compute the packet checksum and return it.
    uint8_t compute_checksum(packet_t* pkt, uint8_t len);
    // Validate a packet's checksum using compute_checksum().
    bool validate_checksum(packet_t* pkt, uint16_t chksum);
    // Perform an encryption handshake with the given message ID.
    void encryption_handshake(uint8_t msg_id);

  private:
    bool is_complete(const std::vector<packet_t*>& frags) const;
    void process_fragments(std::vector<packet_t*>& frags, bool decipher);

  private:
    LoRa* lora;

  private:
    // Initial AES vector for encryption (aligned as required).
    __ALIGN_BEGIN static uint32_t IV_Send[4] __ALIGN_END;
    __ALIGN_BEGIN static uint32_t IV_Receive[4] __ALIGN_END;

    msg_data_callback_t msg_callback = nullptr;

  public:
    // Outgoing and incoming message buffers.
    std::array<message_bucket*, 255> messages;
    std::array<message_bucket*, 255> received;
};

#endif /* INC_PACKET_HANDLER_H_ */
