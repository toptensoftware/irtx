#pragma once

#include <stdint.h>

void setupIrTx();
void pollIrTx();
bool isIrTxBusy();
bool getInflightIrCode(uint32_t* protocol, uint64_t* code);
void handleIrPacket(uint8_t* data, int length);
void handleIrCodePacket(uint8_t* data, int length);
void handleIrCode(uint32_t protocolId, uint64_t code, bool repeat);
