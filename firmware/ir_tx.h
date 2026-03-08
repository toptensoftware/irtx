#pragma once

#include <stdint.h>

void setupIrTx();
void handleIrPacket(uint8_t* data, int length);
void handleIrCodePacket(uint8_t* data, int length);
void handleIrCode(uint16_t irDevIdx, uint32_t protocolId, uint64_t code, bool repeat);
