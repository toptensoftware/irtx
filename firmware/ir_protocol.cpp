#include <Arduino.h>
#include "ir_protocol.h"
#include "config.h"

constexpr IrProtocol protocolNec {
    "nec",
    PROTOCOL_ID_NEC,

    32,
    108000,

    {9000, 4500},   // header
    {562, 0},            // footer

    {562, 1675},    // one
    {562, 562},     // zero

    {9000, 2250, 562, 108000 - 9000 - 2250 - 562}
};

constexpr IrProtocol protocolPana {
    "pana",
    PROTOCOL_ID_PANA,

    48,
    0,

    {3500, 1750},   // header
    {435, 0},            // footer

    {435, 1300},    // one
    {435, 435},     // zero

    {0, 0, 0, 0}
};

const IrProtocol* registeredProtocols[16] = { 
    &protocolNec,
    &protocolPana
};
int registeredProtocolCount = 2;

void registerIrProtocol(const IrProtocol* protocol)
{
    if (registeredProtocolCount < 16) {
        registeredProtocols[registeredProtocolCount++] = protocol;
    }   
}

const IrProtocol* getIrProtocolById(uint32_t id)
{
    for (int i = 0; i < registeredProtocolCount; i++) {
        if (registeredProtocols[i]->id == id) {
            return registeredProtocols[i];
        }
    }
    return nullptr;
}


int getIrProtocolCount()
{
    return registeredProtocolCount;
}

const IrProtocol* getIrProtocolByIndex(int index)
{
    if (index >= 0 && index < registeredProtocolCount) {
        return registeredProtocols[index];
    }
    return nullptr;
}

void statusProtocols()
{
    PRINT("  \"protocols\": [\n");
    for (int i = 0; i < registeredProtocolCount; i++)
    {
        const IrProtocol* p = registeredProtocols[i];
        PRINT("    { \"name\": \"%s\", \"id\": \"0x%08X\", \"bits\": %d }%s\n",
              p->name, p->id, p->bitCount,
              i < registeredProtocolCount - 1 ? "," : "");
    }
    PRINT("  ],\n");
}
