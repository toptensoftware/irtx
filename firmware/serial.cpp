#include <Arduino.h>
#include "serial.h"

void SerialConsole::write(const char* buf, size_t len)
{
    Serial.write((const uint8_t*)buf, len);
}

void SerialConsole::poll()
{
    while (Serial.available())
        feedChar((uint8_t)Serial.read());
}
