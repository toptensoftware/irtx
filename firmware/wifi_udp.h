#pragma once

#include <WiFiUdp.h>

extern WiFiUDP udp;

void setupWifi();
void setupWifiOrAP();
void pollWifi();
void statusWifi();
void startAccessPoint();
