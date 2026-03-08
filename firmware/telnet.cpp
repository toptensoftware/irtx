#include <Arduino.h>
#include <WiFi.h>
#include <lwip/sockets.h>
#include <fcntl.h>
#include "config.h"
#include "log.h"
#include "serial.h"
#include "telnet.h"

#define TELNET_PORT 23

static int serverFd = -1;
static int clientFd = -1;

static char inputBuf[INPUT_MAX];
static int  inputLen  = 0;
static bool lastWasCR = false;

// Telnet IAC state machine
enum IacState { IAC_NONE, IAC_CMD, IAC_OPT, IAC_SB, IAC_SB_IAC };
static IacState iacState = IAC_NONE;

// ---- Internal helpers ----

static void clientSend(const char* buf, size_t len)
{
    if (clientFd >= 0)
        send(clientFd, buf, len, MSG_DONTWAIT);
}

static void clientSendStr(const char* str)
{
    clientSend(str, strlen(str));
}

static void closeClient()
{
    if (clientFd >= 0) { close(clientFd); clientFd = -1; }
    logSetTelnetFd(-1);
    inputLen  = 0;
    lastWasCR = false;
    iacState  = IAC_NONE;
}

static void setupServer()
{
    serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) return;

    int yes = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(TELNET_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0 ||
        listen(serverFd, 1) < 0)
    {
        close(serverFd);
        serverFd = -1;
        return;
    }

    fcntl(serverFd, F_SETFL, O_NONBLOCK);
    LOG("Telnet listening on port %d\n", TELNET_PORT);
}

// ---- Public poll (called from loop) ----

void pollTelnet()
{
    // Lazy setup: wait until WiFi is up
    if (serverFd < 0)
    {
        if (WiFi.status() != WL_CONNECTED) return;
        setupServer();
        if (serverFd < 0) return;
    }

    // Accept new client (only one at a time)
    if (clientFd < 0)
    {
        int fd = accept(serverFd, nullptr, nullptr);
        if (fd >= 0)
        {
            clientFd = fd;
            fcntl(clientFd, F_SETFL, O_NONBLOCK);
            logSetTelnetFd(clientFd);

            // Negotiate: server will echo (WILL ECHO), suppress go-ahead (WILL SGA),
            // and ask client to suppress go-ahead (DO SGA).
            const uint8_t neg[] = {
                255, 251, 1,   // IAC WILL ECHO
                255, 251, 3,   // IAC WILL SUPPRESS-GO-AHEAD
                255, 253, 3,   // IAC DO   SUPPRESS-GO-AHEAD
            };
            clientSend((const char*)neg, sizeof(neg));

            LOG("Telnet client connected\n");
        }
    }

    if (clientFd < 0) return;

    // Read available bytes
    uint8_t buf[64];
    int n = recv(clientFd, buf, sizeof(buf), 0);
    if (n == 0)  { LOG("Telnet client disconnected\n"); closeClient(); return; }
    if (n < 0)   return; // EWOULDBLOCK — nothing to read

    for (int i = 0; i < n; i++)
    {
        uint8_t c = buf[i];

        // ---- IAC (telnet command) filter ----
        switch (iacState)
        {
            case IAC_SB_IAC:
                // Expecting SE (0xF0) to end sub-negotiation
                iacState = (c == 240) ? IAC_NONE : IAC_SB;
                continue;

            case IAC_SB:
                // Inside sub-negotiation: skip until IAC
                if (c == 255) iacState = IAC_SB_IAC;
                continue;

            case IAC_OPT:
                // Option byte after WILL/WONT/DO/DONT
                iacState = IAC_NONE;
                continue;

            case IAC_CMD:
                // Command byte after IAC
                if      (c == 250) iacState = IAC_SB;   // SB — sub-negotiation
                else if (c == 255) iacState = IAC_NONE;  // IAC IAC = literal 0xFF (rare)
                else               iacState = IAC_OPT;   // WILL/WONT/DO/DONT
                continue;

            case IAC_NONE:
                if (c == 255) { iacState = IAC_CMD; continue; }
                break;
        }

        // ---- Normal character handling (mirrors serial.cpp) ----
        if (c == '\n' && lastWasCR) { lastWasCR = false; continue; }
        lastWasCR = (c == '\r');

        if (c == '\r' || c == '\n')
        {
            clientSendStr("\r\n");
            inputBuf[inputLen] = '\0';
            handleCommand(inputBuf);
            inputLen = 0;
        }
        else if ((c == '\b' || c == 127) && inputLen > 0)
        {
            inputLen--;
            clientSendStr("\b \b");
        }
        else if (c >= 32 && inputLen < INPUT_MAX - 1)
        {
            inputBuf[inputLen++] = c;
            char echo[1] = { (char)c };
            clientSend(echo, 1);
        }
    }
}
