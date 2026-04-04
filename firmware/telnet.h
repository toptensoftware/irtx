#pragma once
#include "console.h"

class TelnetConsole : public Console
{
public:
    void poll();
    void write(const char* buf, size_t len) override;
    bool isConnected() const override { return _clientFd >= 0; }

private:
    void setupServer();
    void closeClient();
    bool sendRaw(const char* buf, size_t len);

    int _serverFd = -1;
    int _clientFd = -1;

    enum IacState { IAC_NONE, IAC_CMD, IAC_OPT, IAC_SB, IAC_SB_IAC };
    IacState _iacState = IAC_NONE;
};
