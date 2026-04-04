#pragma once
#include <Arduino.h>
#include "config.h"   // INPUT_MAX, LOG/PRINT macros

// ---- Base Console class ----
//
// Three distinct output paths:
//   Echo   — this->write()       called by feedChar for typed character echo
//   PRINT  — consolePrint()      goes to the console that invoked the current command
//   LOG    — consoleWriteAll()   goes to every connected console + dmesg ring

class Console
{
public:
    // Feed input bytes from the physical channel (serial/socket/websocket).
    // Handles line editing (backspace) and dispatches handleCommand() on Enter.
    void feedChar(uint8_t c);
    void feedBytes(const uint8_t* buf, size_t len);

    // Send output bytes to this specific console's channel.
    // Each subclass is responsible for any framing / line-ending translation.
    virtual void write(const char* buf, size_t len) = 0;

    // True while this console has an active connection (always true for Serial).
    virtual bool isConnected() const = 0;

protected:
    char _buf[INPUT_MAX];
    int  _len       = 0;
    bool _lastWasCR = false;
};

// ---- CaptureConsole ----
// Transient console used by HTTP endpoints to collect PRINT output into a
// String buffer. Set as the active console before calling handleCommand(),
// clear it after, then read .output for the response body.

class CaptureConsole : public Console
{
public:
    String output;
    void write(const char* buf, size_t len) override
    {
        output.concat(buf, (unsigned int)len);
    }
    bool isConnected() const override { return true; }
};

// ---- Console registry ----

// Register a console for LOG fan-out (consoleWriteAll).
// Call once per console instance, before any LOG() output.
void consoleRegister(Console* c);

// LOG path: write to every registered console that is currently connected.
void consoleWriteAll(const char* buf, size_t len);

// PRINT path: write only to the console that invoked the current command.
// No-op when no command is in progress (s_active == nullptr).
void consolePrint(const char* buf, size_t len);

// Set/clear the active console. Called by feedChar() around handleCommand(),
// and by HTTP handlers around CaptureConsole-based command execution.
void consoleSetActive(Console* c);
