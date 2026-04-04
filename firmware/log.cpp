#include <Arduino.h>
#include <stdarg.h>
#include "log.h"
#include "console.h"

// ---- JSON string helper ----

void printJsonString(const char* s)
{
    char buf[512];
    int  pos = 0;
    buf[pos++] = '"';
    for (; *s && pos < (int)sizeof(buf) - 8; s++)
    {
        unsigned char c = (unsigned char)*s;
        if      (c == '"')  { buf[pos++] = '\\'; buf[pos++] = '"';  }
        else if (c == '\\') { buf[pos++] = '\\'; buf[pos++] = '\\'; }
        else if (c < 0x20)  { pos += snprintf(buf + pos, 8, "\\u%04X", c); }
        else                { buf[pos++] = (char)c; }
    }
    buf[pos++] = '"';
    buf[pos]   = '\0';
    printWrite("%s", buf);
}

// ---- dmesg ring buffer ----

#define DMESG_LINES 50
#define DMESG_WIDTH 160

static char dmesgRing[DMESG_LINES][DMESG_WIDTH];
static int  dmesgHead  = 0;   // next slot to write
static int  dmesgCount = 0;   // entries stored (saturates at DMESG_LINES)

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

// Replay the ring buffer to the active console only (PRINT path).
// Uses consolePrint rather than consoleWriteAll so replayed lines are not
// re-stored in dmesgFeed, and the output goes only to the requesting console.
void dmesgPrint()
{
    int start = (dmesgCount < DMESG_LINES) ? 0 : dmesgHead;
    int count =  dmesgCount < DMESG_LINES  ? dmesgCount : DMESG_LINES;
    for (int i = 0; i < count; i++)
    {
        const char* line = dmesgRing[(start + i) % DMESG_LINES];
        consolePrint(line, strlen(line));
        consolePrint("\n", 1);
    }
}

// ---- Core output ----

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
        consoleWriteAll(buf, (size_t)len);
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
        consolePrint(buf, (size_t)len);
}

// ---- Verbose mode ----

static bool logVerbose = false;

void logSetVerbose(bool verbose) { logVerbose = verbose; }
bool logGetVerbose()             { return logVerbose; }

void verboseWrite(const char* fmt, ...)
{
    if (!logVerbose) return;
    char buf[512];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len > 0)
    {
        dmesgFeed(buf, (size_t)len);
        consoleWriteAll(buf, (size_t)len);
    }
}
