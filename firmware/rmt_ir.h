#pragma once

#include <stdint.h>

void setupRmt();
void handleIrPacket(uint8_t* data, int length);
