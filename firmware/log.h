#pragma once

#include <stddef.h>

// LOG path: timestamped, recorded in dmesg, sent to all connected consoles.
void logWrite(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

// PRINT path: sent only to the console that invoked the current command.
// Not recorded in dmesg. Use for command response output.
void printWrite(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

// VERBOSE path: like logWrite but suppressed unless verbose mode is on.
void verboseWrite(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

// Print the last 50 log lines to the active console (PRINT path).
void dmesgPrint();

// Print a JSON-escaped, double-quoted string value via printWrite.
void printJsonString(const char* s);

// Verbose mode toggle.
void logSetVerbose(bool verbose);
bool logGetVerbose();
