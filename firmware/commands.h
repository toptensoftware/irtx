#pragma once

// Execute a null-terminated command line.
// PRINT output goes to the console that set itself as active via consoleSetActive().
// LOG output goes to all connected consoles via consoleWriteAll().
void handleCommand(const char* line);
