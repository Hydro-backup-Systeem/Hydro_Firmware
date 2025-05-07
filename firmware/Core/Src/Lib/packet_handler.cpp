/*
 * PacketHandler.cpp
 *
 *  Created on: Mar 27, 2025
 *    Author: joey
 */

#include <math_utils.h>
#include "packet_handler.h"

#include "main.h"
#include "debug.h"
#include <cstdlib>
#include <stdio.h>
#include <cstring>
#include <algorithm>

#include <memory>

__ALIGN_BEGIN uint32_t PacketHandler::IV_Send[4] __ALIGN_END = {0};
__ALIGN_BEGIN uint32_t PacketHandler::IV_Receive[4] __ALIGN_END = {0};

void swap_endianness(uint32_t* data, std::size_t word_count) {
  for (std::size_t i = 0; i < word_count; ++i) {
    uint32_t v = data[i];
    data[i] = ((v & 0x000000FFu) << 24) |
              ((v & 0x0000FF00u) <<  8) |
              ((v & 0x00FF0000u) >>  8) |
              ((v & 0xFF000000u) >> 24);
  }
}

PacketHandler::PacketHandler(LoRa* lora) : lora(lora), msg_callback(nullptr) {}

void PacketHandler::init() {
  HAL_StatusTypeDef res = HAL_OK;

  // Retrieve and update CRYP configuration for AES encryption.
  CRYP_ConfigTypeDef crypConf;
  res = HAL_CRYP_GetConfig(&hcryp, &crypConf);

  configASSERT(res == HAL_OK);

  crypConf.pInitVect = PacketHandler::IV_Send;
  res = HAL_CRYP_SetConfig(&hcryp, &crypConf);

  configASSERT(res == HAL_OK);

  for (uint8_t i = 0; i < 255; i++) {
    messages[i] = nullptr;
    received[i] = nullptr;
  }
}

void PacketHandler::set_msg_callback(msg_data_callback_t callback) {
  this->msg_callback = callback;
}

bool PacketHandler::is_complete(const std::vector<packet_t*>& frags) const {
  uint8_t total_frags = frags.front()->total_fragments;
  return !frags.empty() && total_frags == frags.size();
}

void PacketHandler::process_fragments(std::vector<packet_t*>& frags, bool decipher) {
  // Get the message type
  auto type = static_cast<PacketTypes>(frags.front()->type);
  
  // Sort the fragments by fragment_id
  std::sort(frags.begin(), frags.end(),
    [](const packet_t* a, const packet_t* b) {
      return a->fragment_id < b->fragment_id;
    }
  );

  // Compute the total length
  uint32_t total = 0;
  for (auto* p : frags) total += p->lenght;

  // Reassamble the fragments
  uint32_t offset = 0;
  std::unique_ptr<uint8_t[]> raw = std::make_unique<uint8_t[]>(total);
  for (auto* p : frags) {
    memcpy(raw.get() + offset, p->data, p->lenght);
    offset += p->lenght;
    delete p;
  }
  frags.clear();

  // Dispatch based on type
  switch (type) {
    case PacketTypes::MSG: {
      if (msg_callback && decipher) {
        // convert raw bytes → big‑endian word array
        uint32_t wordCount = total / 4;
        auto buf32 = std::make_unique<uint32_t[]>(wordCount);
        for (uint32_t i = 0; i < wordCount; ++i) {
          buf32[i] = (uint32_t)raw[4*i + 0] << 24
                   | (uint32_t)raw[4*i + 1] << 16
                   | (uint32_t)raw[4*i + 2] <<  8
                   | (uint32_t)raw[4*i + 3] <<  0;
        }

        // decrypt in‑place
        HAL_CRYP_Decrypt(&hcryp, buf32.get(), wordCount, buf32.get(), 1000);

        // convert decrypted words back to bytes for the callback
        for (uint32_t i = 0; i < wordCount; ++i) {
          raw[4*i + 0] = (buf32[i] >> 24) & 0xFF;
          raw[4*i + 1] = (buf32[i] >> 16) & 0xFF;
          raw[4*i + 2] = (buf32[i] >>  8) & 0xFF;
          raw[4*i + 3] = (buf32[i] >>  0) & 0xFF;
        }
      }

      // finally fire the callback on the byte buffer:
      msg_callback(raw.get(), total);
      break;
    }

    case PacketTypes::ENCRYP: {
      for (int i = 0; i < 4; ++i) {
        uint32_t v =  (uint32_t)raw[4*i + 0] << 24
                    | (uint32_t)raw[4*i + 1] << 16
                    | (uint32_t)raw[4*i + 2] <<  8
                    | (uint32_t)raw[4*i + 3] <<  0;
        IV_Receive[i] = v;
      }

      DEBUGP("IV_Receive: ");
      for (int i = 0; i < 4; ++i) DEBUG_UINT_HEX(" ", IV_Receive[i]);
      DEBUGLN("");

      CRYP_ConfigTypeDef conf;
      HAL_CRYP_GetConfig(&hcryp, &conf);
      conf.pInitVect = IV_Receive;
      HAL_CRYP_SetConfig(&hcryp, &conf);

      break;
    }

    case PacketTypes::ACK:
      break;

    case PacketTypes::NACK:
      break;

    default:
      break;
  }
}

void PacketHandler::poll() {
  for (message_bucket*& bucket : received) {
    if (    bucket == nullptr
        || !is_complete(bucket->frags)) continue;

    process_fragments(bucket->frags, USE_ENCRYPTION);

    delete bucket;
    bucket = nullptr;
  }
}

void PacketHandler::clean() {
  uint32_t now = HAL_GetTick();

  for (message_bucket*& bucket : messages) {
    if (bucket != nullptr && bucket->expiration <= now) {
      for (packet_t* pkt : bucket->frags) {
        delete pkt;
      }

      delete bucket;
      bucket = nullptr;
    }
  }
}

void PacketHandler::encryption_handshake(uint8_t msg_id) {
  HAL_StatusTypeDef res = HAL_OK;

  // Generate random values for the AES IV.
  for (int i = 0; i < 4; i++) {
    res = HAL_RNG_GenerateRandomNumber(&hrng, &PacketHandler::IV_Send[i]);
    if (res != HAL_OK) {
      Error_Handler();
    }
  }

  DEBUGP("IV_Send: ");
  for (int i = 0; i < 4; ++i) DEBUG_UINT_HEX(" ", IV_Send[i]);
  DEBUGLN("");

  // Prepare handshake packet.
  packet_t handshakePkt;
  handshakePkt.type = static_cast<uint8_t>(PacketTypes::ENCRYP);
  handshakePkt.message_id = msg_id;
  handshakePkt.fragment_id = 0;
  handshakePkt.total_fragments = 1;
  handshakePkt.lenght = 4 * sizeof(uint32_t);

  memcpy(handshakePkt.data, PacketHandler::IV_Send, handshakePkt.lenght);
  handshakePkt.checksum = compute_checksum(&handshakePkt, handshakePkt.lenght);
  send_pkt(&handshakePkt);
}

void PacketHandler::send(uint8_t* data, uint32_t size, PacketTypes type) {
  const uint32_t MAX_SIZE = PACKET_MAX_SIZE;

  // 1) Pick a free message ID
  uint8_t id = rand();
  while (messages[id] != nullptr) {
    id = rand();
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  // 2) Create bucket
  message_bucket* bucket = new message_bucket;
  configASSERT(bucket);
  bucket->expiration = HAL_GetTick() + 10000;
  messages[id] = bucket;

  // 3) Encryption handshake
  encryption_handshake(id);

  // 4) Pad to 16‑byte boundary for AES
  uint32_t paddedSizeBytes = ((size + 15) / 16) * 16;
  uint32_t paddedWordCount = paddedSizeBytes / 4;

  // 5) Allocate temporary buffers
  std::unique_ptr<uint8_t[]> raw_plain  = std::make_unique<uint8_t[]>(paddedSizeBytes);
  std::unique_ptr<uint32_t[]> word_buff = std::make_unique<uint32_t[]>(paddedWordCount);

  // 6) PKCS#7 padding:
  uint8_t padding_value = (uint8_t)paddedSizeBytes - size;

  memset(raw_plain.get(), padding_value, paddedSizeBytes);
  memcpy(raw_plain.get(), data, size);

  // Pack raw_plain into word_buff (big-endian words)
  for (uint32_t i = 0; i < paddedWordCount; i++) {
    word_buff[i] = (uint32_t)raw_plain[4*i + 0] << 24
                 | (uint32_t)raw_plain[4*i + 1] << 16
                 | (uint32_t)raw_plain[4*i + 2] <<  8
                 | (uint32_t)raw_plain[4*i + 3] <<  0;
  }

  // Make sure we are using the correct IV
  CRYP_ConfigTypeDef conf;
  HAL_CRYP_GetConfig(&hcryp, &conf);

  conf.pInitVect = IV_Send;
  HAL_CRYP_SetConfig(&hcryp, &conf);

  // 7) Encrypt in place
  HAL_StatusTypeDef crypRes = HAL_CRYP_Encrypt(
    &hcryp,
    word_buff.get(),
    paddedWordCount,
    word_buff.get(),
    1000
  );

  configASSERT(crypRes == HAL_OK);

  // Unpack the buffer
  auto raw_cipher = std::move(raw_plain);
  for (uint32_t i = 0; i < paddedWordCount; i++) {
    raw_cipher[4*i + 0] = (word_buff[i] >> 24) & 0xFF;
    raw_cipher[4*i + 1] = (word_buff[i] >> 16) & 0xFF;
    raw_cipher[4*i + 2] = (word_buff[i] >>  8) & 0xFF;
    raw_cipher[4*i + 3] = (word_buff[i] >>  0) & 0xFF;
  }

  DEBUGP("Cipher:");
  for (uint32_t i = 0; i < paddedSizeBytes; i++) {
    DEBUG_UINT_HEX(" ", raw_cipher.get()[i]);
    if ((i+1)%16==0) DEBUGLN("");
  }
  DEBUGLN("");

  // 8) Fragment and build packets
  uint32_t offset       = 0;
  uint32_t remaining    = paddedSizeBytes;
  uint32_t rawFragments = (paddedSizeBytes + MAX_SIZE - 1) / MAX_SIZE;

  configASSERT(rawFragments <= 0xFF);

  uint8_t fragIdx        = 0;
  uint8_t totalFragments = static_cast<uint8_t>(rawFragments);

  while (remaining) {
    // determine this fragment’s size
    uint32_t packetLen = (remaining > MAX_SIZE) ? MAX_SIZE : remaining;

    // bounds checks
    configASSERT(packetLen > 0);
    configASSERT(packetLen <= MAX_SIZE);
    configASSERT(offset + packetLen <= paddedSizeBytes);

    // allocate and populate
    packet_t* pkt = new packet_t;
    configASSERT(pkt);

    pkt->type            = static_cast<uint8_t>(type);
    pkt->message_id      = id;
    pkt->fragment_id     = fragIdx;
    pkt->total_fragments = totalFragments;
    pkt->lenght          = static_cast<uint8_t>(packetLen);
    configASSERT(pkt->lenght == packetLen);

    // safe copy into pkt->data
    configASSERT(packetLen <= sizeof(pkt->data));
    memcpy(pkt->data,
           raw_cipher.get() + offset,
           packetLen);

    pkt->checksum = compute_checksum(pkt, packetLen);

    bucket->frags.push_back(pkt);

    // advance
    fragIdx++;
    offset    += packetLen;
    remaining -= packetLen;
  }

  for (auto pkt : bucket->frags) {
    send_pkt(pkt);
  }
}


void PacketHandler::receive() {
  uint8_t error = 0;
  uint8_t rx_buffer[PACKET_MAX_SIZE + 9];

  // Blocking call with a 500ms timeout.
  uint8_t rx_received = lora->receive(rx_buffer, sizeof(rx_buffer), 500, &error);

  if (rx_received == 0 || error != LORA_OK) {
    return;
  }

  // Validate header bytes.
  if (rx_buffer[0] != 0xA5 || rx_buffer[1] != 0x5A) {
    return;
  }

  packet_t* pkt = new packet_t;
  if (!pkt) {
    return;
  }

  pkt->type = rx_buffer[2];
  pkt->message_id = rx_buffer[3];
  pkt->fragment_id = rx_buffer[4];
  pkt->total_fragments = rx_buffer[5];
  pkt->lenght = rx_buffer[6];
  memcpy(pkt->data, &rx_buffer[7], pkt->lenght);

  if (pkt->lenght > PACKET_MAX_SIZE) {
    delete pkt;
    return;
  }

  uint8_t received_checksum = rx_buffer[pkt->lenght + 8];
  if (!validate_checksum(pkt, received_checksum)) {
    delete pkt;
    return;
  }

  message_bucket* bucket = received[pkt->message_id];
  if (bucket == nullptr) {
    bucket = new message_bucket;
    received[pkt->message_id] = bucket;
  }

  bucket->frags.push_back(pkt);
}

void PacketHandler::send_pkt(packet_t* pkt) {
  uint8_t buffer[pkt->lenght + 9];
  buffer[0] = 0xA5;
  buffer[1] = 0x5A;
  buffer[2] = pkt->type;
  buffer[3] = pkt->message_id;
  buffer[4] = pkt->fragment_id;
  buffer[5] = pkt->total_fragments;
  buffer[6] = pkt->lenght;
  memcpy(&buffer[7], pkt->data, pkt->lenght);
  buffer[pkt->lenght + 8] = pkt->checksum;

  bool succes = false;

  while (!succes) {
    succes = lora->send(buffer, pkt->lenght + 9);
    vTaskDelay(pdMS_TO_TICKS(50));
  }

  vTaskDelay(pdMS_TO_TICKS(50));
}

uint8_t PacketHandler::compute_checksum(packet_t* pkt, uint8_t len) {
  uint16_t sum = 0;
  uint8_t* bytes = pkt->data;

  for (size_t i = 0; i < len; ++i) {
    sum += bytes[i];
  }

  return static_cast<uint8_t>(~sum);
}

bool PacketHandler::validate_checksum(packet_t* pkt, uint16_t chksum) {
  // Recompute checksum and compare.
  uint8_t computed = compute_checksum(pkt, pkt->lenght);
  return computed == chksum;
}
