#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include "activities_types.h"
#include "activities.h"
#include "ir_tx.h"
#include "wifi_udp.h"
#include "led.h"

// ---- BPAK loader ----

#define BPAK_SIGNATURE  0x4B415042u
#define BPAK_HEADER_SIZE 32

static uint8_t*      activitiesData   = nullptr;
activitiesRoot*      activitiesConfig = nullptr;

// ---- Device on/off state ----
// Bit i = device i is considered on (updated eagerly when ops are enqueued).
static uint32_t s_deviceOnMask = 0;

// ---- Current activity ----
static int s_currentActivity = 0;

// ---- Modifier key state ----
#define MODIFIER_TIMEOUT_MS 5000
static uint32_t      s_modifierProtocol = 0;
static uint64_t      s_modifierValue    = 0;
static unsigned long s_modifierExpiry   = 0;

// ---- Internal "commit activity" op ----
// Used as the last item enqueued during a switch so that s_currentActivity
// is updated only after all transition ops have executed.
#define OP_INTERNAL_COMMIT 0xFFu
struct CommitOp { op base; uint32_t newIndex; };
static CommitOp s_commitOp;

// ---- Op ring buffer ----
#define OP_QUEUE_SIZE 32
static op* s_opQueue[OP_QUEUE_SIZE];
static int s_opHead = 0;   // next slot to dequeue
static int s_opTail = 0;   // next slot to enqueue

static bool isQueueEmpty() { return s_opHead == s_opTail; }
static bool isQueueFull()  { return ((s_opTail + 1) % OP_QUEUE_SIZE) == s_opHead; }

static void enqueueOp(op* o)
{
    if (isQueueFull())
    {
        LOG("Activities: queue full, dropping op\n");
        return;
    }
    s_opQueue[s_opTail] = o;
    s_opTail = (s_opTail + 1) % OP_QUEUE_SIZE;
}

void enqueueOps(op** ops, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
        if (ops[i]) enqueueOp(ops[i]);
}

// ---- Async op execution state ----
static op*           s_currentOp      = nullptr;
static unsigned long s_opEndMs        = 0;
// LED blink tracking
static unsigned long s_ledNextToggle  = 0;
static bool          s_ledOn          = true;

// ---- Start an op (called when it is dequeued) ----
// Instant ops execute fully and leave s_currentOp == nullptr.
// Timed ops set s_currentOp so pollCurrentOp() is called each cycle.
static void startOp(op* o)
{
    switch (o->op)
    {
        case OP_SEND_IR:
        {
            sendIrOp* sio = (sendIrOp*)o;
            if (sio->ipAddr == 0)
            {
                handleIrCode(0, sio->protocol, sio->irCode, false);
            }
            else
            {
                // Forward to a remote IRTX device via UDP cmd=4
                uint8_t  pkt[17];
                uint16_t cmd    = 4;
                uint16_t devIdx = 0;
                uint8_t  repeat = 0;
                memcpy(pkt,      &cmd,           2);
                memcpy(pkt + 2,  &devIdx,        2);
                memcpy(pkt + 4,  &sio->protocol, 4);
                memcpy(pkt + 8,  &sio->irCode,   8);
                memcpy(pkt + 16, &repeat,        1);
                IPAddress ip(sio->ipAddr & 0xFF,
                            (sio->ipAddr >> 8)  & 0xFF,
                            (sio->ipAddr >> 16) & 0xFF,
                            (sio->ipAddr >> 24) & 0xFF);
                udp.beginPacket(ip, UDP_PORT);
                udp.write(pkt, sizeof(pkt));
                udp.endPacket();
                VERBOSE("Activities: IR -> %s 0x%08X:0x%016llX\n",
                        ip.toString().c_str(), sio->protocol, sio->irCode);
            }
            break;
        }

        case OP_SEND_WOL:
        {
            sendWolOp* wol = (sendWolOp*)o;
            uint8_t magic[102];
            memset(magic, 0xFF, 6);
            for (int i = 0; i < 16; i++)
                memcpy(magic + 6 + i * 6, wol->macaddr, 6);
            udp.beginPacket(IPAddress(255, 255, 255, 255), 9);
            udp.write(magic, sizeof(magic));
            udp.endPacket();
            LOG("Activities: WoL -> %02X:%02X:%02X:%02X:%02X:%02X\n",
                wol->macaddr[0], wol->macaddr[1], wol->macaddr[2],
                wol->macaddr[3], wol->macaddr[4], wol->macaddr[5]);
            break;
        }

        case OP_HTTP_GET:
        {
            // NOTE: HTTPClient.GET() blocks until the response is received.
            httpGetOp* hg = (httpGetOp*)o;
            if (WiFi.status() == WL_CONNECTED)
            {
                HTTPClient http;
                http.begin(hg->url);
                int code = http.GET();
                LOG("Activities: HTTP GET %s -> %d\n", hg->url, code);
                http.end();
            }
            else
            {
                LOG("Activities: HTTP GET skipped (WiFi not connected)\n");
            }
            break;
        }

        case OP_HTTP_POST:
        {
            // NOTE: HTTPClient.POST() blocks until the response is received.
            httpPostOp* hp = (httpPostOp*)o;
            if (WiFi.status() == WL_CONNECTED)
            {
                HTTPClient http;
                http.begin(hp->url);
                if (hp->contentType)     http.addHeader("Content-Type",     hp->contentType);
                if (hp->contentEncoding) http.addHeader("Content-Encoding", hp->contentEncoding);
                int code = http.POST(hp->data, hp->data_count);
                LOG("Activities: HTTP POST %s -> %d\n", hp->url, code);
                http.end();
            }
            else
            {
                LOG("Activities: HTTP POST skipped (WiFi not connected)\n");
            }
            break;
        }

        case OP_UDP_PACKET:
        {
            // TODO: udpPacketOp has no port field; using UDP_PORT until the struct is extended.
            udpPacketOp* up = (udpPacketOp*)o;
            IPAddress ip(up->ipAddr & 0xFF,
                        (up->ipAddr >> 8)  & 0xFF,
                        (up->ipAddr >> 16) & 0xFF,
                        (up->ipAddr >> 24) & 0xFF);
            udp.beginPacket(ip, UDP_PORT);
            udp.write(up->data, up->data_count);
            udp.endPacket();
            VERBOSE("Activities: UDP -> %s (%d bytes)\n",
                    ip.toString().c_str(), (int)up->data_count);
            break;
        }

        case OP_DELAY:
        {
            delayOp* d = (delayOp*)o;
            s_currentOp = o;
            s_opEndMs   = millis() + d->duration;
            VERBOSE("Activities: delay %dms\n", (int)d->duration);
            break;
        }

        case OP_LED:
        {
            ledOp* lo = (ledOp*)o;
            uint8_t r = (lo->color >> 16) & 0xFF;
            uint8_t g = (lo->color >> 8)  & 0xFF;
            uint8_t b =  lo->color        & 0xFF;
            ledColor(r, g, b);
            if (lo->duration > 0)
            {
                s_currentOp     = o;
                s_opEndMs       = millis() + lo->duration;
                s_ledOn         = true;
                s_ledNextToggle = (lo->period > 0) ? millis() + lo->period / 2 : 0;
            }
            break;
        }

        case OP_SWITCH_ACTIVITY:
        {
            switchActivityOp* sa = (switchActivityOp*)o;
            switchActivity((int)sa->index);
            break;
        }

        case OP_INTERNAL_COMMIT:
        {
            CommitOp* co = (CommitOp*)o;
            s_currentActivity = (int)co->newIndex;
            LOG("Activities: active '%s'\n",
                activitiesConfig->activities[s_currentActivity].name);
            break;
        }

        default:
            LOG("Activities: unknown op %d\n", (int)o->op);
            break;
    }
}

// ---- Poll a timed op — returns true when complete ----
static bool pollCurrentOp()
{
    if (!s_currentOp) return true;

    unsigned long now = millis();

    switch (s_currentOp->op)
    {
        case OP_DELAY:
            return now >= s_opEndMs;

        case OP_LED:
        {
            if (now >= s_opEndMs)
            {
                ledColor(0, 4, 0);  // restore idle green
                return true;
            }
            ledOp* lo = (ledOp*)s_currentOp;
            if (lo->period > 0 && s_ledNextToggle > 0 && now >= s_ledNextToggle)
            {
                s_ledOn = !s_ledOn;
                if (s_ledOn)
                    ledColor((lo->color >> 16) & 0xFF,
                             (lo->color >> 8)  & 0xFF,
                              lo->color        & 0xFF);
                else
                    ledColor(0, 0, 0);
                s_ledNextToggle = now + lo->period / 2;
            }
            return false;
        }

        default:
            return true;
    }
}

// ---- Helper: is devIdx in an activity's device list? ----
static bool deviceInActivity(const activity* act, int32_t devIdx)
{
    if (!act->devices) return false;
    for (uint32_t i = 0; i < act->devices_count; i++)
        if (act->devices[i] == devIdx) return true;
    return false;
}

// ---- Switch activity ----
void switchActivity(int newIndex)
{
    if (!activitiesConfig)
    {
        LOG("Activities: no config loaded\n");
        return;
    }
    if (newIndex < 0 || (uint32_t)newIndex >= activitiesConfig->activities_count)
    {
        LOG("Activities: invalid activity index %d\n", newIndex);
        return;
    }
    if (newIndex == s_currentActivity)
        return;  // no-op
    if (!isQueueEmpty())
    {
        LOG("Activities: queue busy, ignoring switch to %d\n", newIndex);
        return;
    }

    activity* oldAct = &activitiesConfig->activities[s_currentActivity];
    activity* newAct = &activitiesConfig->activities[newIndex];

    LOG("Activities: '%s' -> '%s'\n", oldAct->name, newAct->name);

    // 1. willDeactivate (old)
    enqueueOps(oldAct->willDeactivateOps, oldAct->willDeactivateOps_count);

    // 2. willActivate (new)
    enqueueOps(newAct->willActivateOps, newAct->willActivateOps_count);

    // 3. Turn on devices new activity needs that are not already on.
    //    Null device list = this activity doesn't manage devices.
    if (newAct->devices != nullptr)
    {
        for (uint32_t i = 0; i < newAct->devices_count; i++)
        {
            int32_t devIdx = newAct->devices[i];
            if (devIdx < 0 || devIdx >= MAX_ACTIVITIES_DEVICES ||
                (uint32_t)devIdx >= activitiesConfig->devices_count)
                continue;
            if (!(s_deviceOnMask & (1u << devIdx)))
            {
                enqueueOps(activitiesConfig->devices[devIdx].onOps,
                           activitiesConfig->devices[devIdx].onOps_count);
                s_deviceOnMask |= (1u << devIdx);
            }
        }
    }

    // 4. Turn off devices old activity had that new no longer needs.
    if (oldAct->devices != nullptr)
    {
        for (uint32_t i = 0; i < oldAct->devices_count; i++)
        {
            int32_t devIdx = oldAct->devices[i];
            if (devIdx < 0 || devIdx >= MAX_ACTIVITIES_DEVICES ||
                (uint32_t)devIdx >= activitiesConfig->devices_count)
                continue;
            if (!deviceInActivity(newAct, devIdx) && (s_deviceOnMask & (1u << devIdx)))
            {
                enqueueOps(activitiesConfig->devices[devIdx].offOps,
                           activitiesConfig->devices[devIdx].offOps_count);
                s_deviceOnMask &= ~(1u << devIdx);
            }
        }
    }

    // 5. didActivate (new)
    enqueueOps(newAct->didActivateOps, newAct->didActivateOps_count);

    // 6. didDeactivate (old)
    enqueueOps(oldAct->didDectivateOps, oldAct->didDectivateOps_count);

    // 7. Commit: s_currentActivity updated once all transition ops have run.
    s_commitOp.base.op = OP_INTERNAL_COMMIT;
    s_commitOp.newIndex = (uint32_t)newIndex;
    enqueueOp((op*)&s_commitOp);
}

// ---- Binding matcher (called from IR RX on every decoded code) ----
void applyActivityBinding(uint32_t protocol, uint64_t value)
{
    if (!activitiesConfig || activitiesConfig->activities_count == 0) return;

    activity*     act = &activitiesConfig->activities[s_currentActivity];
    unsigned long now = millis();

    // 1. If a modifier is pending, look for a modifier+key binding first.
    if (s_modifierProtocol != 0 && now < s_modifierExpiry)
    {
        for (uint32_t i = 0; i < act->bindings_count; i++)
        {
            binding* b = &act->bindings[i];
            if (b->protocol == protocol &&
                b->modifier  == s_modifierValue &&
                b->value     == value)
            {
                VERBOSE("Activities: modifier+key binding matched\n");
                s_modifierProtocol = 0;
                enqueueOps(b->ops, b->ops_count);
                return;
            }
        }
    }

    // 2. Look for an unmodified binding.
    for (uint32_t i = 0; i < act->bindings_count; i++)
    {
        binding* b = &act->bindings[i];
        if (b->protocol == protocol && b->modifier == 0 && b->value == value)
        {
            VERBOSE("Activities: binding matched\n");
            enqueueOps(b->ops, b->ops_count);
            return;
        }
    }

    // 3. Check if this key acts as a modifier for any binding in this activity.
    for (uint32_t i = 0; i < act->bindings_count; i++)
    {
        binding* b = &act->bindings[i];
        if (b->modifier != 0 && b->protocol == protocol && b->modifier == value)
        {
            VERBOSE("Activities: modifier key armed 0x%016llX (timeout %ds)\n",
                    value, MODIFIER_TIMEOUT_MS / 1000);
            s_modifierProtocol = protocol;
            s_modifierValue    = value;
            s_modifierExpiry   = now + MODIFIER_TIMEOUT_MS;
            return;
        }
    }
}

// ---- Status ----
void statusActivities()
{
    PRINT("--- Activities ---\n");
    if (!activitiesConfig)
    {
        PRINT("  no config loaded\n");
        return;
    }
    PRINT("  devices (%d):\n", (int)activitiesConfig->devices_count);
    for (uint32_t i = 0; i < activitiesConfig->devices_count; i++)
    {
        bool on = (s_deviceOnMask & (1u << i)) != 0;
        PRINT("    [%d] %s (%s)\n", (int)i,
              activitiesConfig->devices[i].name, on ? "on" : "off");
    }
    PRINT("  activities (%d):\n", (int)activitiesConfig->activities_count);
    for (uint32_t i = 0; i < activitiesConfig->activities_count; i++)
    {
        PRINT("    [%d] %s%s\n", (int)i,
              activitiesConfig->activities[i].name,
              (int)i == s_currentActivity ? " *" : "");
    }
    PRINT("  queue: %s\n", isQueueEmpty() ? "empty" : "busy");
}

// ---- BPAK loader ----
static bool loadActivities()
{
    File f = LittleFS.open("/activities.bin", "r");
    if (!f)
    {
        LOG("Activities: no activities.bin found\n");
        return false;
    }

    size_t size = f.size();
    if (size < BPAK_HEADER_SIZE)
    {
        LOG("Activities: file too small (%d bytes)\n", (int)size);
        f.close();
        return false;
    }

    uint8_t* data = (uint8_t*)malloc(size);
    if (!data)
    {
        LOG("Activities: failed to allocate %d bytes\n", (int)size);
        f.close();
        return false;
    }

    f.readBytes((char*)data, size);
    f.close();

    uint32_t* hdr = (uint32_t*)data;
    if (hdr[0] != BPAK_SIGNATURE)
    {
        LOG("Activities: bad signature 0x%08X\n", (unsigned)hdr[0]);
        free(data);
        return false;
    }

    uint32_t relocCount  = hdr[2];
    uint32_t relocOffset = hdr[3];
    uint32_t base        = (uint32_t)data;
    uint32_t* relocs     = (uint32_t*)(data + relocOffset);
    for (uint32_t i = 0; i < relocCount; i++)
    {
        uint32_t* ptrField = (uint32_t*)(data + relocs[i]);
        *ptrField += base;
    }

    activitiesData   = data;
    activitiesConfig = (activitiesRoot*)(data + BPAK_HEADER_SIZE);

    LOG("Activities: loaded %d device(s), %d activity(s)\n",
        (int)activitiesConfig->devices_count,
        (int)activitiesConfig->activities_count);
    return true;
}

// ---- Activate the initial (index 0) activity on boot ----
static void activateInitial()
{
    if (!activitiesConfig || activitiesConfig->activities_count == 0) return;

    s_currentActivity = 0;
    activity* act = &activitiesConfig->activities[0];

    enqueueOps(act->willActivateOps, act->willActivateOps_count);

    if (act->devices != nullptr)
    {
        for (uint32_t i = 0; i < act->devices_count; i++)
        {
            int32_t devIdx = act->devices[i];
            if (devIdx < 0 || devIdx >= MAX_ACTIVITIES_DEVICES ||
                (uint32_t)devIdx >= activitiesConfig->devices_count)
                continue;
            enqueueOps(activitiesConfig->devices[devIdx].onOps,
                       activitiesConfig->devices[devIdx].onOps_count);
            s_deviceOnMask |= (1u << devIdx);
        }
    }

    enqueueOps(act->didActivateOps, act->didActivateOps_count);
    LOG("Activities: initial activity '%s'\n", act->name);
}

// ---- Public API ----

void setupActivities()
{
    if (loadActivities())
        activateInitial();
}

bool reloadActivities()
{
    // Reset all runtime state before reloading.
    s_opHead = s_opTail = 0;
    s_currentOp        = nullptr;
    s_deviceOnMask     = 0;
    s_currentActivity  = 0;
    s_modifierProtocol = 0;

    free(activitiesData);
    activitiesData   = nullptr;
    activitiesConfig = nullptr;

    if (loadActivities())
    {
        activateInitial();
        return true;
    }
    return false;
}

void pollActivities()
{
    // Expire a pending modifier key.
    if (s_modifierProtocol != 0 && millis() >= s_modifierExpiry)
    {
        VERBOSE("Activities: modifier key expired\n");
        s_modifierProtocol = 0;
    }

    // Poll the currently-running timed op (delay, LED, …).
    if (s_currentOp != nullptr)
    {
        if (pollCurrentOp())
            s_currentOp = nullptr;
        return;  // at most one op per cycle
    }

    // Dequeue and start the next op.
    if (!isQueueEmpty())
    {
        op* next = s_opQueue[s_opHead];
        s_opHead = (s_opHead + 1) % OP_QUEUE_SIZE;
        startOp(next);
    }
}
