#pragma once

#include <Preferences.h>
#include "log.h"

// ---- Logging ----
// Timestamped log — output goes to Serial and any connected telnet client.
#define LOG(fmt, ...)   logWrite("[%7.3f] " fmt, millis() / 1000.0f, ##__VA_ARGS__)
// Plain print — same fan-out as LOG but NOT recorded in dmesg. Use for command responses.
#define PRINT(fmt, ...) printWrite(fmt, ##__VA_ARGS__)
// Verbose log — like LOG but suppressed unless verbose mode is on. Recorded in dmesg when emitted.
#define VERBOSE(fmt, ...) verboseWrite("[%7.3f] " fmt, millis() / 1000.0f, ##__VA_ARGS__)
#define VERBOSE2(fmt, ...) verboseWrite(fmt, ##__VA_ARGS__)

// ---- Hardware ----
#define UDP_PORT     4210
#define CARRIER_FREQ 38000   // 38kHz IR carrier

// ---- Limits ----
#define MAX_TIMING_VALUES  512
#define IR_HEADER_SIZE     12  // uint16 cmd + uint16 unused + uint32 carrierFreq + uint32 gap
#define INPUT_MAX          256

extern Preferences prefs;

void nvsDump();
void nvsReset();