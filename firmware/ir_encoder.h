#pragma once

#include "ir_protocol.h"

void ir_encode(const IrProtocol& protocol, uint64_t code, bool repeat, int* signal, int* count);
void ir_encode(const IrProtocol& protocol, uint64_t code, bool repeat, uint16_t* signal, int* count, int* pGap);