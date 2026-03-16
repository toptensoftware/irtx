#pragma once

#include <stdint.h>

void setupIrTx();
void pollIrTx();
void handleIrPacket(uint8_t* data, int length);
void handleIrCodePacket(uint8_t* data, int length);
void handleIrCode(uint32_t protocolId, uint64_t code, bool repeat);
