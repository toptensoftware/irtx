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

// Print a JSON-escaped, double-quoted string value via printWrite.
void printJsonString(const char* s);

// Capture all output to a String for the duration of a synchronous operation.
// Call logStartCapture() before, logEndCapture() after; output still goes to
// Serial/telnet as normal.
void logStartCapture(String* buf);
void logEndCapture();

// Called by telnet.cpp when a client connects or disconnects.
void logSetTelnetFd(int fd);

// Verbose mode — logWrite only if verbose is on. Recorded in dmesg.
void verboseWrite(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void logSetVerbose(bool verbose);
bool logGetVerbose();
