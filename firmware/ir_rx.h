#pragma once

#include <stdint.h>

enum class IrEventKind
{
    Press,
    Repeat,
    LongPress,
    Release,
};

void onIrEvent(uint32_t protocol_id, uint64_t code, IrEventKind kind);

void setupIrRx();
void pollIrRx();
