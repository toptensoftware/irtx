#pragma once

#include <stdint.h>

enum class IrEventKind : uint32_t
{
    Press     = 1,
    Repeat    = 2,
    LongPress = 4,
    Release   = 8,
};

void onIrEvent(uint32_t protocol_id, uint64_t code, IrEventKind kind);

void setupIrRx();
void pollIrRx();
void suppressRelease();
void setIrRepeatRate(uint32_t ms);
void simulateIrRx(uint32_t protocol, uint64_t code, uint32_t eventKindMask);
