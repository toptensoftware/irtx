#pragma once

#include <WiFiUdp.h>

extern WiFiUDP udp;

void setupWifi();
void pollWifi();
void statusWifi();
