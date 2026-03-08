#include <Arduino.h>
#include <lwip/sockets.h>
#include <stdarg.h>
#include "log.h"

// ---- Telnet fan-out ----

static int telnetFd = -1;

void logSetTelnetFd(int fd)
{
    telnetFd = fd;
}

// ---- dmesg ring buffer ----

#define DMESG_LINES 50
#define DMESG_WIDTH 160

static char dmesgRing[DMESG_LINES][DMESG_WIDTH];
static int  dmesgHead  = 0;   // next slot to write
static int  dmesgCount = 0;   // entries stored (saturates at DMESG_LINES)

// Accumulate characters into a line; commit to ring on '\n'
static char dmesgAccum[DMESG_WIDTH];
static int  dmesgAccumLen = 0;

static void dmesgFeed(const char* buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        char c = buf[i];
        if (c == '\n')
        {
            if (dmesgAccumLen > 0)
            {
                dmesgAccum[dmesgAccumLen] = '\0';
                strncpy(dmesgRing[dmesgHead], dmesgAccum, DMESG_WIDTH - 1);
                dmesgRing[dmesgHead][DMESG_WIDTH - 1] = '\0';
                dmesgHead = (dmesgHead + 1) % DMESG_LINES;
                if (dmesgCount < DMESG_LINES) dmesgCount++;
                dmesgAccumLen = 0;
            }
        }
        else if (c >= 32 && dmesgAccumLen < DMESG_WIDTH - 1)
        {
            dmesgAccum[dmesgAccumLen++] = c;
        }
    }
}

void dmesgPrint()
{
    int start = (dmesgCount < DMESG_LINES) ? 0 : dmesgHead;
    int count =  dmesgCount < DMESG_LINES  ? dmesgCount : DMESG_LINES;
    for (int i = 0; i < count; i++)
    {
        const char* line = dmesgRing[(start + i) % DMESG_LINES];
        // Write directly to outputs, bypassing dmesgFeed to avoid re-storing replayed lines
        Serial.println(line);
        if (telnetFd >= 0)
        {
            send(telnetFd, line, strlen(line), MSG_DONTWAIT);
            send(telnetFd, "\r\n", 2, MSG_DONTWAIT);
        }
    }
}

// ---- Core output ----

void logWriteRaw(const char* buf, size_t len)
{
    Serial.write((const uint8_t*)buf, len);

    if (telnetFd < 0) return;

    // Translate \n -> \r\n for telnet client
    const char* p   = buf;
    const char* end = buf + len;
    while (p < end)
    {
        const char* nl = (const char*)memchr(p, '\n', end - p);
        if (!nl)
        {
            if (send(telnetFd, p, end - p, MSG_DONTWAIT) < 0)
                telnetFd = -1;
            break;
        }
        if (nl > p && send(telnetFd, p, nl - p, MSG_DONTWAIT) < 0)
            { telnetFd = -1; break; }
        if (send(telnetFd, "\r\n", 2, MSG_DONTWAIT) < 0)
            { telnetFd = -1; break; }
        p = nl + 1;
    }
}

void logWrite(const char* fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0)
    {
        dmesgFeed(buf, (size_t)len);
        logWriteRaw(buf, (size_t)len);
    }
}

void printWrite(const char* fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0)
        logWriteRaw(buf, (size_t)len);
}
