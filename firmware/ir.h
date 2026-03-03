#pragma once

#include <stdint.h>

void setupIr();
void handleIrPacket(uint8_t* data, int length);
