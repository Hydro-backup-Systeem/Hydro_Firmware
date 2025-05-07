/*
 * packet.h
 *
 *  Created on: Mar 25, 2025
 *      Author: joey
 */

#ifndef INC_PACKET_H_
#define INC_PACKET_H_

#include <stdint.h>

#define PACKET_MAX_SIZE 119

enum class PacketTypes {
    MSG = 0,    // Sending a new message
    ENCRYP, // Send counter for AES over plain text

    ACK,  // Acknowledge a whole message
    NACK, // Not Acknowledge, data should be list of missing fragments
};

typedef struct {
    const uint16_t header = 0xA55A;

    uint8_t type;
    uint8_t message_id;
    uint8_t fragment_id;
    uint8_t total_fragments;

    uint8_t lenght;
    uint8_t data[PACKET_MAX_SIZE];

    uint16_t checksum;
} packet_t;

#endif /* INC_PACKET_H_ */
