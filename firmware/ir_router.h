#pragma once

#include <stdint.h>

// ---- IR Routing ----
//
// Routes a decoded IR code to a target action based on a mapping table.
// Mappings: SOURCE_PROTOCOL:SOURCE_IRCODE -> TARGET_PROTOCOL:TARGET_IRCODE:TARGET_IP
//
// TARGET_PROTOCOL == 0       → suppress (do nothing)
// TARGET_IP       == 0.0.0.0 → retransmit locally via IR TX
// TARGET_IP       != 0.0.0.0 → forward to remote device via UDP (cmd=4)
//
// UDP packet (cmd=5) to set the full route table (replaces all existing routes):
//   [uint16 cmd=5][uint16 count][entry * count]
// Each entry (28 bytes, all fields little-endian):
//   [uint32 srcProtocol][uint64 srcCode][uint32 dstProtocol][uint64 dstCode][uint32 dstIp]
// dstIp is stored with first octet in LSB (e.g. 192.168.1.1 → 0x0101A8C0).

void handleRoutePacket(uint8_t* data, int length);
void applyRoutes(uint32_t srcProtocol, uint64_t srcCode);
void statusRoutes();
