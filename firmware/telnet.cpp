#include <Arduino.h>
#include <WiFi.h>
#include <lwip/sockets.h>
#include <fcntl.h>
#include "config.h"
#include "log.h"
#include "telnet.h"

#define TELNET_PORT 23

// ---- Internal helpers ----

bool TelnetConsole::sendRaw(const char* buf, size_t len)
{
    if (_clientFd < 0) return false;
    if (send(_clientFd, buf, len, MSG_DONTWAIT) < 0) { closeClient(); return false; }
    return true;
}

// Translate \n → \r\n for telnet, but leave already-paired \r\n untouched.
void TelnetConsole::write(const char* buf, size_t len)
{
    const char* p   = buf;
    const char* end = buf + len;
    while (p < end)
    {
        const char* nl = (const char*)memchr(p, '\n', end - p);
        if (!nl)
        {
            sendRaw(p, end - p);
            break;
        }
        // Send bytes before the \n
        if (nl > p && !sendRaw(p, nl - p)) return;
        // If the \n is already preceded by \r (within this same buffer), send \n
        // as-is; otherwise insert \r before it.
        bool already_cr = (nl > buf) && (*(nl - 1) == '\r');
        if (already_cr)
        {
            if (!sendRaw("\n", 1)) return;
        }
        else
        {
            if (!sendRaw("\r\n", 2)) return;
        }
        p = nl + 1;
    }
}

void TelnetConsole::closeClient()
{
    if (_clientFd >= 0) { close(_clientFd); _clientFd = -1; }
    _iacState = IAC_NONE;
    _len      = 0;
    _lastWasCR = false;
}

void TelnetConsole::setupServer()
{
    _serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_serverFd < 0) return;

    int yes = 1;
    setsockopt(_serverFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(TELNET_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(_serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0 ||
        listen(_serverFd, 1) < 0)
    {
        close(_serverFd); _serverFd = -1; return;
    }

    fcntl(_serverFd, F_SETFL, O_NONBLOCK);
    LOG("Telnet listening on port %d\n", TELNET_PORT);
}

// ---- Public poll (called from loop) ----

void TelnetConsole::poll()
{
    // Lazy setup: wait until WiFi is up (STA connected or AP active)
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

            // Negotiate: server will echo (WILL ECHO), suppress go-ahead (WILL SGA),
            // and ask client to suppress go-ahead (DO SGA).
            const uint8_t neg[] = {
                255, 251, 1,   // IAC WILL ECHO
                255, 251, 3,   // IAC WILL SUPPRESS-GO-AHEAD
                255, 253, 3,   // IAC DO   SUPPRESS-GO-AHEAD
            };
            sendRaw((const char*)neg, sizeof(neg));
            LOG("Telnet client connected\n");
        }
    }

    if (_clientFd < 0) return;

    uint8_t buf[64];
    int n = recv(_clientFd, buf, sizeof(buf), 0);
    if (n == 0) { LOG("Telnet client disconnected\n"); closeClient(); return; }
    if (n < 0)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK) { LOG("Telnet client error\n"); closeClient(); }
        return;
    }

    for (int i = 0; i < n; i++)
    {
        uint8_t c = buf[i];

        // ---- IAC (telnet command) filter ----
        switch (_iacState)
        {
            case IAC_SB_IAC:
                _iacState = (c == 240) ? IAC_NONE : IAC_SB;
                continue;
            case IAC_SB:
                if (c == 255) _iacState = IAC_SB_IAC;
                continue;
            case IAC_OPT:
                _iacState = IAC_NONE;
                continue;
            case IAC_CMD:
                if      (c == 250) _iacState = IAC_SB;
                else if (c == 255) _iacState = IAC_NONE;
                else               _iacState = IAC_OPT;
                continue;
            case IAC_NONE:
                if (c == 255) { _iacState = IAC_CMD; continue; }
                break;
        }

        feedChar(c);
    }
}
