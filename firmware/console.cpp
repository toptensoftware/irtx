#include <Arduino.h>
#include "console.h"
#include "commands.h"

// ---- Console registry ----

static Console* s_consoles[4] = {};
static int      s_count       = 0;
static Console* s_active      = nullptr;

void consoleRegister(Console* c)
{
    if (s_count < (int)(sizeof(s_consoles) / sizeof(s_consoles[0])))
        s_consoles[s_count++] = c;
}

void consoleSetActive(Console* c) { s_active = c; }

void consoleWriteAll(const char* buf, size_t len)
{
    for (int i = 0; i < s_count; i++)
        if (s_consoles[i]->isConnected())
            s_consoles[i]->write(buf, len);
}

void consolePrint(const char* buf, size_t len)
{
    if (s_active && s_active->isConnected())
        s_active->write(buf, len);
}

// ---- Console::feedChar ----

void Console::feedChar(uint8_t c)
{
    // Collapse CRLF pairs to a single line event
    if (c == '\n' && _lastWasCR) { _lastWasCR = false; return; }
    _lastWasCR = (c == '\r');

    if (c == '\r' || c == '\n')
    {
        write("\r\n", 2);           // echo newline back to this console
        _buf[_len] = '\0';
        consoleSetActive(this);     // PRINT targets this console for the duration
        handleCommand(_buf);
        consoleSetActive(nullptr);
        _len = 0;
    }
    else if ((c == '\b' || c == 127) && _len > 0)
    {
        _len--;
        write("\b \b", 3);         // erase character on this console
    }
    else if (c >= 32 && c != 127 && _len < INPUT_MAX - 1)
    {
        _buf[_len++] = (char)c;
        write((const char*)&c, 1); // echo typed character back to this console
    }
}

void Console::feedBytes(const uint8_t* buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
        feedChar(buf[i]);
}
