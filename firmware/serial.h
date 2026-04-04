#pragma once
#include "console.h"

class SerialConsole : public Console
{
public:
    void poll();
    void write(const char* buf, size_t len) override;
    bool isConnected() const override { return true; }
};
