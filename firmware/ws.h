#pragma once
#include "console.h"

class WsConsole : public Console
{
public:
    void poll();
    void write(const char* buf, size_t len) override;
    bool isConnected() const override { return _open; }

private:
    void setupServer();
    void closeClient();
    bool doHandshake();
    void processRecvByte(uint8_t b);
    void onFrameComplete();
    void sendFrame(uint8_t opcode, const char* payload, size_t len);
    bool sendRaw(const char* buf, size_t len);

    int  _serverFd = -1;
    int  _clientFd = -1;
    bool _open     = false;

    // HTTP upgrade handshake accumulation buffer
    char _hsBuf[1024];
    int  _hsLen = 0;

    // WebSocket frame receive state machine
    enum FrameState { FS_HDR0, FS_HDR1, FS_LEN16, FS_MASK, FS_PAYLOAD };
    FrameState _frameState  = FS_HDR0;
    uint8_t    _opcode      = 0;
    bool       _masked      = false;
    uint32_t   _payloadLen  = 0;
    uint32_t   _payloadRcvd = 0;
    uint8_t    _extBuf[2]   = {};
    int        _extPos      = 0;
    uint8_t    _maskKey[4]  = {};
};
