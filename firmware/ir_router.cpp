#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "ir_router.h"
#include "ir_tx.h"
#include "wifi_udp.h"

// ---- Route Table ----

#define MAX_ROUTES 64

// Entry size: uint32 srcProtocol + uint64 srcCode + uint32 dstProtocol + uint64 dstCode + uint32 dstIp
#define ROUTE_ENTRY_SIZE 28

struct IrRoute {
    uint32_t srcProtocol;
    uint64_t srcCode;
    uint32_t dstProtocol;   // 0 = suppress
    uint64_t dstCode;
    uint32_t dstIp;         // little-endian; 0 = local retransmit
};

static IrRoute routes[MAX_ROUTES];
static int     routeCount = 0;

// ---- Configure Routes via UDP ----
// Packet: [uint16 cmd=5][uint16 count][entry * count]

void handleRoutePacket(uint8_t* data, int length)
{
    if (length < 4) return;

    uint16_t count;
    memcpy(&count, data + 2, 2);

    int needed = 4 + (int)count * ROUTE_ENTRY_SIZE;
    if (length < needed) {
        LOG("Route: packet too short for %d entries (got %d, need %d)\n", count, length, needed);
        return;
    }
    if (count > MAX_ROUTES) {
        LOG("Route: too many entries (%d, max %d)\n", count, MAX_ROUTES);
        return;
    }

    uint8_t* p = data + 4;
    for (int i = 0; i < count; i++) {
        memcpy(&routes[i].srcProtocol, p,  4); p += 4;
        memcpy(&routes[i].srcCode,     p,  8); p += 8;
        memcpy(&routes[i].dstProtocol, p,  4); p += 4;
        memcpy(&routes[i].dstCode,     p,  8); p += 8;
        memcpy(&routes[i].dstIp,       p,  4); p += 4;
    }
    routeCount = count;
    LOG("Route: loaded %d route(s)\n", routeCount);
}

// ---- Apply Routes on Decode ----

void applyRoutes(uint32_t srcProtocol, uint64_t srcCode)
{
    for (int i = 0; i < routeCount; i++) 
    {
        const IrRoute& r = routes[i];
        if (r.srcProtocol != srcProtocol || r.srcCode != srcCode)
            continue;

        VERBOSE("Routing: 0x%08X:0x%016llX\n", srcProtocol, srcCode);

        if (r.dstProtocol == 0) 
        {
            // Suppress — log and skip
            VERBOSE(" - suppressed\n");
            continue;
        }

        if (r.dstIp == 0) 
        {
            // Local retransmit
            VERBOSE(" - local TX 0x%08X:0x%016llX\n", r.dstProtocol, r.dstCode);
            handleIrCode(0, r.dstProtocol, r.dstCode, false);
            continue;
        }

        // Remote retransmit: send cmd=4 packet to target IP
        // Layout: [uint16 cmd=4][uint16 devIdx=0][uint32 protocol][uint64 code][uint8 repeat=0]
        uint8_t pkt[17];
        uint16_t cmd    = 4;
        uint16_t devIdx = 0;
        uint8_t  repeat = 0;
        memcpy(pkt,      &cmd,           2);
        memcpy(pkt + 2,  &devIdx,        2);
        memcpy(pkt + 4,  &r.dstProtocol, 4);
        memcpy(pkt + 8,  &r.dstCode,     8);
        memcpy(pkt + 16, &repeat,        1);

        // dstIp is little-endian: first octet in LSB
        IPAddress ip(r.dstIp & 0xFF,
                     (r.dstIp >> 8)  & 0xFF,
                     (r.dstIp >> 16) & 0xFF,
                     (r.dstIp >> 24) & 0xFF);
        VERBOSE(" - remote TX to %s 0x%08X:0x%016llX\n", ip.toString().c_str(), r.dstProtocol, r.dstCode);
        udp.beginPacket(ip, UDP_PORT);
        udp.write(pkt, sizeof(pkt));
        udp.endPacket();
    }
}

// ---- Status ----

void statusRoutes()
{
    PRINT("--- IR routes (%d) ---\n", routeCount);
    for (int i = 0; i < routeCount; i++) 
    {
        const IrRoute& r = routes[i];
        if (r.dstProtocol == 0) 
        {
            PRINT("  0x%08X:0x%016llX -> suppress\n",
                r.srcProtocol, r.srcCode);
        } 
        else if (r.dstIp == 0) 
        {
            PRINT("  0x%08X:0x%016llX -> local 0x%08X:0x%016llX\n",
                r.srcProtocol, r.srcCode, r.dstProtocol, r.dstCode);
        } 
        else 
        {
            IPAddress ip(r.dstIp & 0xFF,
                         (r.dstIp >> 8)  & 0xFF,
                         (r.dstIp >> 16) & 0xFF,
                         (r.dstIp >> 24) & 0xFF);
            PRINT("  0x%08X:0x%016llX -> %s 0x%08X:0x%016llX\n",
                r.srcProtocol, r.srcCode, ip.toString().c_str(), r.dstProtocol, r.dstCode);
        }
    }
}
