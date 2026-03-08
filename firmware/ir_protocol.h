#pragma once

#include <stdint.h>

#define RIFF_CODE(a, b, c, d) \
    ((uint32_t)(a)        | \
    ((uint32_t)(b) << 8)  | \
    ((uint32_t)(c) << 16) | \
    ((uint32_t)(d) << 24))


#define PROTOCOL_ID_NEC RIFF_CODE('N', 'E', 'C', ' ')
#define PROTOCOL_ID_PANA RIFF_CODE('P', 'A', 'N', 'A')

struct IrPulse
{
    int pulse;
    int space;
};

struct IrProtocol
{
    const char* name;
    uint32_t id;
    int bitCount;
    int frameLength;

    IrPulse header;
    IrPulse footer;

    IrPulse one;
    IrPulse zero;

    int repeat[4];
};

extern const IrProtocol protocolNec;
extern const IrProtocol protocolPana;

void registerIrProtocol(const IrProtocol* protocol);
const IrProtocol* getIrProtocolById(uint32_t id);
int getIrProtocolCount();
const IrProtocol* getIrProtocolByIndex(int index);
void statusProtocols();

