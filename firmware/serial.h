#pragma once

// Handle a null-terminated command line. Output goes via LOG/PRINT (Serial + telnet).
void handleCommand(const char* line);

void pollSerial();
