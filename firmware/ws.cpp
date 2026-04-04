#include <Arduino.h>
#include <WiFi.h>
#include <lwip/sockets.h>
#include <fcntl.h>
#include <mbedtls/sha1.h>
#include <mbedtls/base64.h>
#include "config.h"
#include "log.h"
#include "ws.h"

#define WS_PORT 81

// ---- SHA-1 + base64 for WebSocket handshake ----

static bool computeAcceptKey(const char* wsKey, char* out, size_t outSize)
{
    char combined[128];
    snprintf(combined, sizeof(combined),
             "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", wsKey);

    uint8_t hash[20];
    mbedtls_sha1((const unsigned char*)combined, strlen(combined), hash);

    size_t outLen = 0;
    if (mbedtls_base64_encode((unsigned char*)out, outSize, &outLen,
                               hash, sizeof(hash)) != 0)
        return false;
    out[outLen] = '\0';  // mbedtls_base64_encode does not null-terminate
    return true;
}

// ---- Internal helpers ----

bool WsConsole::sendRaw(const char* buf, size_t len)
{
    if (_clientFd < 0) return false;
    if (send(_clientFd, buf, len, MSG_DONTWAIT) < 0) { closeClient(); return false; }
    return true;
}

void WsConsole::sendFrame(uint8_t opcode, const char* payload, size_t len)
{
    uint8_t hdr[4];
    int     hdrLen;
    hdr[0] = 0x80 | (opcode & 0x0F);  // FIN + opcode, no fragmentation

    if (len < 126)
    {
        hdr[1]  = (uint8_t)len;
        hdrLen  = 2;
    }
    else
    {
        hdr[1]  = 126;
        hdr[2]  = (uint8_t)(len >> 8);
        hdr[3]  = (uint8_t)(len & 0xFF);
        hdrLen  = 4;
    }

    if (!sendRaw((const char*)hdr, hdrLen)) return;
    if (len > 0) sendRaw(payload, len);
}

// Output path: wrap bytes in a WebSocket text frame.
// xterm.js is configured with convertEol:true so \n is handled client-side;
// no \n→\r\n translation needed here.
void WsConsole::write(const char* buf, size_t len)
{
    if (!_open || len == 0) return;
    sendFrame(0x1, buf, len);
}

void WsConsole::closeClient()
{
    if (_clientFd >= 0) { close(_clientFd); _clientFd = -1; }
    _open       = false;
    _hsLen      = 0;
    _frameState = FS_HDR0;
    _len        = 0;
    _lastWasCR  = false;
}

void WsConsole::setupServer()
{
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0) return;

    int yes = 1;
    setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(WS_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0 ||
        listen(_serverFd, 1) < 0)
    {
        close(_serverFd); _serverFd = -1; return;
    }

    fcntl(_serverFd, F_SETFL, O_NONBLOCK);
    LOG("WebSocket listening on port %d\n", WS_PORT);
}

// Accumulate the HTTP upgrade request and send the 101 response.
// Returns true once the handshake is complete.
bool WsConsole::doHandshake()
{
    uint8_t buf[64];
    int n = recv(_clientFd, buf, sizeof(buf), 0);
    if (n == 0) { closeClient(); return false; }
    if (n < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK) closeClient();
        return false;
    }

    if (_hsLen + n > (int)sizeof(_hsBuf) - 1) { closeClient(); return false; }
    memcpy(_hsBuf + _hsLen, buf, n);
    _hsLen += n;
    _hsBuf[_hsLen] = '\0';

    // Wait until we have the full HTTP request (ends with blank line)
    if (!strstr(_hsBuf, "\r\n\r\n")) return false;

    // Extract Sec-WebSocket-Key header value
    const char* keyHeader = "Sec-WebSocket-Key: ";
    const char* keyStart  = strstr(_hsBuf, keyHeader);
    if (!keyStart) { closeClient(); return false; }
    keyStart += strlen(keyHeader);
    const char* keyEnd = strstr(keyStart, "\r\n");
    if (!keyEnd)   { closeClient(); return false; }

    char wsKey[64];
    int  keyLen = (int)(keyEnd - keyStart);
    if (keyLen <= 0 || keyLen >= (int)sizeof(wsKey)) { closeClient(); return false; }
    memcpy(wsKey, keyStart, keyLen);
    wsKey[keyLen] = '\0';

    // Compute Sec-WebSocket-Accept = base64(SHA1(key + WS_GUID))
    char acceptKey[64];
    if (!computeAcceptKey(wsKey, acceptKey, sizeof(acceptKey))) { closeClient(); return false; }

    char response[256];
    int  rLen = snprintf(response, sizeof(response),
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: %s\r\n"
        "\r\n",
        acceptKey);
    send(_clientFd, response, rLen, MSG_DONTWAIT);

    _open       = true;
    _hsLen      = 0;
    _frameState = FS_HDR0;
    LOG("WebSocket client connected\n");
    return true;
}

// ---- WebSocket frame receive state machine ----
// Processes one byte at a time; unmasks payload and feeds text bytes to
// feedChar() which handles line editing and command dispatch.

void WsConsole::processRecvByte(uint8_t b)
{
    switch (_frameState)
    {
        case FS_HDR0:
            _opcode     = b & 0x0F;
            _frameState = FS_HDR1;
            break;

        case FS_HDR1:
            _masked     = (b & 0x80) != 0;
            _payloadLen = b & 0x7F;
            if (_payloadLen == 127)
            {
                // 64-bit length is not expected for terminal input; close.
                closeClient();
            }
            else if (_payloadLen == 126)
            {
                _extPos     = 0;
                _frameState = FS_LEN16;
            }
            else if (_masked)
            {
                _extPos     = 0;
                _frameState = FS_MASK;
            }
            else
            {
                _payloadRcvd = 0;
                _frameState  = (_payloadLen > 0) ? FS_PAYLOAD : FS_HDR0;
                if (_payloadLen == 0) onFrameComplete();
            }
            break;

        case FS_LEN16:
            _extBuf[_extPos++] = b;
            if (_extPos == 2)
            {
                _payloadLen = ((uint32_t)_extBuf[0] << 8) | _extBuf[1];
                if (_masked)
                {
                    _extPos     = 0;
                    _frameState = FS_MASK;
                }
                else
                {
                    _payloadRcvd = 0;
                    _frameState  = (_payloadLen > 0) ? FS_PAYLOAD : FS_HDR0;
                    if (_payloadLen == 0) onFrameComplete();
                }
            }
            break;

        case FS_MASK:
            _maskKey[_extPos++] = b;
            if (_extPos == 4)
            {
                _payloadRcvd = 0;
                _frameState  = (_payloadLen > 0) ? FS_PAYLOAD : FS_HDR0;
                if (_payloadLen == 0) onFrameComplete();
            }
            break;

        case FS_PAYLOAD:
            if (_masked) b ^= _maskKey[_payloadRcvd % 4];

            // Text and binary frames: feed unmasked byte to line editor
            if (_opcode == 0x1 || _opcode == 0x2)
                feedChar(b);

            if (++_payloadRcvd >= _payloadLen)
            {
                onFrameComplete();
                _frameState = FS_HDR0;
            }
            break;
    }
}

void WsConsole::onFrameComplete()
{
    if (_opcode == 0x8)       // Close — echo close frame and disconnect
    {
        sendFrame(0x8, nullptr, 0);
        closeClient();
    }
    else if (_opcode == 0x9)  // Ping → Pong
    {
        sendFrame(0xA, nullptr, 0);
    }
}

// ---- Public poll (called from loop) ----

void WsConsole::poll()
{
    // Lazy setup: wait until WiFi is up
    if (_serverFd < 0)
    {
        if (WiFi.status() != WL_CONNECTED && WiFi.getMode() != WIFI_AP) return;
        setupServer();
        if (_serverFd < 0) return;
    }

    // Accept new client (only one at a time)
    if (_clientFd < 0)
    {
        int fd = accept(_serverFd, nullptr, nullptr);
        if (fd >= 0)
        {
            _clientFd = fd;
            fcntl(_clientFd, F_SETFL, O_NONBLOCK);
            _open  = false;
            _hsLen = 0;
        }
    }

    if (_clientFd < 0) return;

    // Handshake phase: accumulate HTTP upgrade request
    if (!_open)
    {
        doHandshake();
        return;
    }

    // Frame receive phase
    uint8_t buf[64];
    int n = recv(_clientFd, buf, sizeof(buf), 0);
    if (n == 0) { LOG("WebSocket client disconnected\n"); closeClient(); return; }
    if (n < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK) { LOG("WebSocket client error\n"); closeClient(); }
        return;
    }

    for (int i = 0; i < n; i++)
        processRecvByte(buf[i]);
}
