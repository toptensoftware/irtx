#pragma once

#include <stddef.h>

// Write formatted text to Serial and any connected telnet client. Recorded in dmesg.
void logWrite(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

// Same output as logWrite but NOT recorded in dmesg. Use for command responses.
void printWrite(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

// Write raw bytes to Serial and any connected telnet client.
// Newlines (\n) are transparently translated to \r\n for the telnet client.
void logWriteRaw(const char* buf, size_t len);

// Print the last 50 log lines (ring buffer).
void dmesgPrint();

// Called by telnet.cpp when a client connects or disconnects.
void logSetTelnetFd(int fd);
