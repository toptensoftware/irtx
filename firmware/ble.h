#pragma once

void setupBle();
void pollBle();
void statusBle();
void blePair(int slot);
void bleUnpair(int slot);
void bleConnect(int slot);
void handleBleConnectPacket(uint8_t* data, int length);
void handleBleHidPacket(uint8_t* data, int length);
