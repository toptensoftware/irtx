#pragma once

#include <WiFiUdp.h>

extern WiFiUDP udp;

void setupWifi();
void handleUdpPacket(uint8_t* data, int length);
void pollUdp();
