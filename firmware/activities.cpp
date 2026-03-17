#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "config.h"
#include "activities_types.h"
#include "activities.h"
#include "ir_rx.h"
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

// ---- Hold time tracking ----
static uint32_t      s_holdProtocol = 0;
static uint64_t      s_holdValue    = 0;
static unsigned long s_holdStartMs  = 0;

// ---- Internal "commit activity" op ----
// Used as the last item enqueued during a switch so that s_currentActivity
// is updated only after all transition ops have executed.
#define OP_INTERNAL_COMMIT 0xFFu
struct CommitOp { op base; uint32_t newIndex; };
static CommitOp s_commitOp;

// ---- Heap-allocated op flag ----
// OR'd into op::op for ops allocated at runtime (not from the binary blob).
// Such ops are freed by the queue drainer after execution.
#define OP_FLAG_HEAP 0x80000000u
#define OP_CODE(o)   ((o)->op & ~OP_FLAG_HEAP)

// ---- Registers ----
static struct { uint32_t protocol; uint64_t irCode; } s_irReg = {0, 0};
static String  s_stringReg;
static bool    s_boolReg = false;

// ---- Background HTTP task ----
// Each request is assigned a monotonically-increasing generation number.
// Only the most-recently-started request's response is retained; older
// in-flight requests are treated as fire-and-forget and their responses
// discarded.  waitHttpOp polls until s_httpDoneGen == s_httpCurrentGen.
struct HttpTaskParams
{
    uint32_t gen;
    bool     isPost;
    char*    url;
    uint8_t* data;
    uint32_t dataLen;
    char*    contentType;
    char*    contentEncoding;
};

static SemaphoreHandle_t s_httpMutex       = nullptr;
static uint32_t          s_httpCurrentGen  = 0;  // gen of most recently started request
static uint32_t          s_httpDoneGen     = 0;  // gen of most recently captured response
static String            s_httpTaskResponse;

static void httpTaskFn(void* param)
{
    HttpTaskParams* p = (HttpTaskParams*)param;
    String response;
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;
        http.begin(p->url);
        int code;
        if (p->isPost)
        {
            if (p->contentType)     http.addHeader("Content-Type",     p->contentType);
            if (p->contentEncoding) http.addHeader("Content-Encoding", p->contentEncoding);
            code = http.POST(p->data, (int)p->dataLen);
        }
        else
        {
            code = http.GET();
            if (code > 0) response = http.getString();
        }
        http.end();
    }
    else
    {
        LOG("Activities: HTTP op skipped (WiFi not connected)\n");
    }

    // Only retain the response if this is still the most recent request.
    xSemaphoreTake(s_httpMutex, portMAX_DELAY);
    if (p->gen == s_httpCurrentGen)
    {
        s_httpTaskResponse = response;
        s_httpDoneGen      = p->gen;
    }
    xSemaphoreGive(s_httpMutex);

    free(p->url);
    free(p->data);
    free(p->contentType);
    free(p->contentEncoding);
    free(p);
    vTaskDelete(nullptr);
}

static void spawnHttpTask(HttpTaskParams* p)
{
    // Assign next generation under the mutex; any older in-flight task becomes
    // fire-and-forget (its response will be discarded on arrival).
    xSemaphoreTake(s_httpMutex, portMAX_DELAY);
    p->gen = ++s_httpCurrentGen;
    xSemaphoreGive(s_httpMutex);

    // Log it
    if (p->isPost)
    {
        VERBOSE("Activities: HTTP POST #%u: %s\n", p->gen, p->url);
    }
    else
    {
        VERBOSE("Activities: HTTP GET #%u: %s\n", p->gen, p->url);
    }

    if (xTaskCreate(httpTaskFn, "http_op", 4096, p, 1, nullptr) != pdPASS)
    {
        LOG("Activities: failed to create HTTP task\n");
        // Roll back so waitHttpOp doesn't stall forever waiting for a task
        // that never started.
        xSemaphoreTake(s_httpMutex, portMAX_DELAY);
        s_httpDoneGen = s_httpCurrentGen;
        xSemaphoreGive(s_httpMutex);
        free(p->url);
        free(p->data);
        free(p->contentType);
        free(p->contentEncoding);
        free(p);
    }
}

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

// Insert ops at the HEAD of the queue so they execute before already-queued ops.
// Used by ifTrueOp to inject the chosen branch ahead of any remaining ops.
// Iterates in reverse so that ops end up in forward order after insertion.
static void injectOpsAtHead(op** ops, uint32_t count)
{
    for (int i = (int)count - 1; i >= 0; i--)
    {
        if (!ops[i]) continue;
        int newHead = (s_opHead - 1 + OP_QUEUE_SIZE) % OP_QUEUE_SIZE;
        if (newHead == s_opTail)
        {
            LOG("Activities: if-branch overflow, dropping remaining ops\n");
            break;
        }
        s_opHead = newHead;
        s_opQueue[s_opHead] = ops[i];
    }
}

// ---- Async op execution state ----
static op*           s_currentOp      = nullptr;
static unsigned long s_opEndMs        = 0;
// LED blink tracking
static unsigned long s_ledNextToggle  = 0;
static bool          s_ledOn          = true;

// ---- Op handler function types ----
typedef void (*startOpFn)(op*);
typedef bool (*pollOpFn)(op*);

// ---- Individual start-op handlers ----

static void startSendIrOp(op* o)
{
    // Defer execution to pollSendIrOp so we wait until IR TX is free.
    s_currentOp = o;
}

static bool pollSendIrOp(op* o)
{
    if (isIrTxBusy()) return false;

    sendIrOp* sio = (sendIrOp*)o;
    // protocol == 0 means use the IR code register (pass-through).
    uint32_t proto = sio->protocol ? sio->protocol : s_irReg.protocol;
    uint64_t code  = sio->protocol ? sio->irCode   : s_irReg.irCode;
    if (sio->ipAddr == 0)
    {
        handleIrCode(proto, code, false);
    }
    else
    {
        // Forward to a remote IRTX device via UDP cmd=4
        uint8_t  pkt[17];
        uint16_t cmd    = 4;
        uint16_t unused = 0;
        uint8_t  repeat = 0;
        memcpy(pkt,      &cmd,    2);
        memcpy(pkt + 2,  &unused, 2);
        memcpy(pkt + 4,  &proto,  4);
        memcpy(pkt + 8,  &code,   8);
        memcpy(pkt + 16, &repeat, 1);
        IPAddress ip(sio->ipAddr & 0xFF,
                    (sio->ipAddr >> 8)  & 0xFF,
                    (sio->ipAddr >> 16) & 0xFF,
                    (sio->ipAddr >> 24) & 0xFF);
        udp.beginPacket(ip, UDP_PORT);
        udp.write(pkt, sizeof(pkt));
        udp.endPacket();
        VERBOSE("Activities: IR -> %s 0x%08X:0x%016llX\n",
                ip.toString().c_str(), proto, code);
    }
    return true;
}

static void startSendWolOp(op* o)
{
    sendWolOp* wol = (sendWolOp*)o;
    uint8_t magic[102];
    memset(magic, 0xFF, 6);
    for (int i = 0; i < 16; i++)
        memcpy(magic + 6 + i * 6, wol->macaddr, 6);
    udp.beginPacket(IPAddress(255, 255, 255, 255), 9);
    udp.write(magic, sizeof(magic));
    udp.endPacket();
    VERBOSE("Activities: WoL -> %02X:%02X:%02X:%02X:%02X:%02X\n",
        wol->macaddr[0], wol->macaddr[1], wol->macaddr[2],
        wol->macaddr[3], wol->macaddr[4], wol->macaddr[5]);
}

static void startHttpGetOp(op* o)
{
    httpGetOp* hg = (httpGetOp*)o;
    HttpTaskParams* p = (HttpTaskParams*)calloc(1, sizeof(HttpTaskParams));
    if (!p) { LOG("Activities: OOM for HTTP task\n"); return; }
    p->isPost = false;
    p->url    = strdup(hg->url);
    if (!p->url) { free(p); LOG("Activities: OOM for HTTP task\n"); return; }
    spawnHttpTask(p);
}

static void startHttpPostOp(op* o)
{
    httpPostOp* hp = (httpPostOp*)o;
    HttpTaskParams* p = (HttpTaskParams*)calloc(1, sizeof(HttpTaskParams));
    if (!p) { LOG("Activities: OOM for HTTP task\n"); return; }
    p->isPost = true;
    p->url    = strdup(hp->url);
    if (!p->url) { free(p); LOG("Activities: OOM for HTTP task\n"); return; }
    if (hp->data_count > 0 && hp->data)
    {
        p->data = (uint8_t*)malloc(hp->data_count);
        if (!p->data) { free(p->url); free(p); LOG("Activities: OOM for HTTP task\n"); return; }
        memcpy(p->data, hp->data, hp->data_count);
        p->dataLen = hp->data_count;
    }
    if (hp->contentType)     p->contentType     = strdup(hp->contentType);
    if (hp->contentEncoding) p->contentEncoding = strdup(hp->contentEncoding);
    spawnHttpTask(p);
}

static void startUdpPacketOp(op* o)
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
}

static void startDelayOp(op* o)
{
    delayOp* d = (delayOp*)o;
    s_currentOp = o;
    s_opEndMs   = millis() + d->duration;
    VERBOSE("Activities: delay %dms\n", (int)d->duration);
}

static void startLedOp(op* o)
{
    ledOp* lo = (ledOp*)o;
    setLed(LED_PRIORITY_USER, lo->color);
    if (lo->duration > 0)
    {
        s_currentOp     = o;
        s_opEndMs       = millis() + lo->duration;
        s_ledOn         = true;
        s_ledNextToggle = (lo->period > 0) ? millis() + lo->period / 2 : 0;
    }
}

static void startSwitchActivityOp(op* o)
{
    switchActivityOp* sa = (switchActivityOp*)o;
    switchActivity((int)sa->index);
}

static void startSetIrRegOp(op* o)
{
    setIrRegOp* sr = (setIrRegOp*)o;
    s_irReg.protocol = sr->protocol;
    s_irReg.irCode   = sr->irCode;
}

static void startSearchStringOp(op* o)
{
    searchStringOp* ss = (searchStringOp*)o;
    s_boolReg = s_stringReg.indexOf(ss->matchString) >= 0;
    VERBOSE("Activities: search '%s' -> %s\n",
        ss->matchString, s_boolReg ? "true" : "false");
}

static void startIfTrueOp(op* o)
{
    ifTrueOp* it = (ifTrueOp*)o;
    if (s_boolReg)
        injectOpsAtHead(it->trueOps,  it->trueOps_count);
    else
        injectOpsAtHead(it->falseOps, it->falseOps_count);
}

static void startWaitHttpOp(op* o)
{
    // Become a timed op; pollWaitHttpOp drives completion.
    s_currentOp = o;
}

static bool pollWaitHttpOp(op* /*o*/)
{
    bool done = false;
    if (xSemaphoreTake(s_httpMutex, 0) == pdTRUE)
    {
        if (s_httpCurrentGen == 0)
        {
            // No HTTP request has ever been fired.
            done = true;
        }
        else if (s_httpDoneGen == s_httpCurrentGen)
        {
            // The most recently started request has completed.
            s_stringReg        = s_httpTaskResponse;
            s_httpTaskResponse = "";
            done = true;
        }
        xSemaphoreGive(s_httpMutex);
    }
    return done;
}

static void startCommitOp(op* o)
{
    CommitOp* co = (CommitOp*)o;
    s_currentActivity = (int)co->newIndex;
    VERBOSE("Activities: active '%s'\n",
        activitiesConfig->activities[s_currentActivity].name);
}

// ---- Individual poll-op handlers (return true when the op is complete) ----

static bool pollDelayOp(op* /*o*/)
{
    return millis() >= s_opEndMs;
}

static bool pollLedOp(op* o)
{
    unsigned long now = millis();
    if (now >= s_opEndMs)
    {
        setLed(LED_PRIORITY_USER, 0xFFFFFFFF);
        return true;
    }
    ledOp* lo = (ledOp*)o;
    if (lo->period > 0 && s_ledNextToggle > 0 && now >= s_ledNextToggle)
    {
        s_ledOn = !s_ledOn;
        if (s_ledOn)
            setLed(LED_PRIORITY_USER, lo->color);
        else
            setLed(LED_PRIORITY_USER, 0xFFFFFFFF);
        s_ledNextToggle = now + lo->period / 2;
    }
    return false;
}

// ---- Dispatch tables (indexed by op code; index 0 unused) ----
// OP_INTERNAL_COMMIT (0xFF) is out of range and handled separately in startOp().

static const startOpFn s_startOpTable[] = {
    nullptr,               // 0 — unused
    startSendIrOp,         // OP_SEND_IR         = 1
    startSendWolOp,        // OP_SEND_WOL        = 2
    startHttpGetOp,        // OP_HTTP_GET         = 3
    startHttpPostOp,       // OP_HTTP_POST        = 4
    startUdpPacketOp,      // OP_UDP_PACKET       = 5
    startDelayOp,          // OP_DELAY            = 6
    startLedOp,            // OP_LED              = 7
    startSwitchActivityOp, // OP_SWITCH_ACTIVITY  = 8
    startSetIrRegOp,       // OP_SET_IR_REG       = 9
    startSearchStringOp,   // OP_SEARCH_STRING    = 10
    startIfTrueOp,         // OP_IF_TRUE          = 11
    startWaitHttpOp,       // OP_WAIT_HTTP        = 12
};
static const uint32_t START_OP_TABLE_SIZE = sizeof(s_startOpTable) / sizeof(s_startOpTable[0]);

static const pollOpFn s_pollOpTable[] = {
    nullptr,          // 0 — unused
    pollSendIrOp,     // OP_SEND_IR         = 1
    nullptr,          // OP_SEND_WOL        = 2
    nullptr,          // OP_HTTP_GET         = 3
    nullptr,          // OP_HTTP_POST        = 4
    nullptr,          // OP_UDP_PACKET       = 5
    pollDelayOp,      // OP_DELAY            = 6
    pollLedOp,        // OP_LED              = 7
    nullptr,          // OP_SWITCH_ACTIVITY  = 8
    nullptr,          // OP_SET_IR_REG       = 9
    nullptr,          // OP_SEARCH_STRING    = 10
    nullptr,          // OP_IF_TRUE          = 11
    pollWaitHttpOp,   // OP_WAIT_HTTP        = 12
};
static const uint32_t POLL_OP_TABLE_SIZE = sizeof(s_pollOpTable) / sizeof(s_pollOpTable[0]);

// ---- Start an op (called when it is dequeued) ----
// Instant ops execute fully and leave s_currentOp == nullptr.
// Timed ops set s_currentOp so pollCurrentOp() is called each cycle.
static void startOp(op* o)
{
    uint32_t code = OP_CODE(o);

    // OP_INTERNAL_COMMIT is an internal op outside the public table range.
    if (code == OP_INTERNAL_COMMIT)
    {
        startCommitOp(o);
        return;
    }

    if (code >= START_OP_TABLE_SIZE || s_startOpTable[code] == nullptr)
    {
        LOG("Activities: unknown op %d\n", (int)code);
        return;
    }

    s_startOpTable[code](o);
}

// ---- Poll a timed op — returns true when complete ----
static bool pollCurrentOp()
{
    if (!s_currentOp) return true;

    uint32_t code = OP_CODE(s_currentOp);
    if (code < POLL_OP_TABLE_SIZE && s_pollOpTable[code] != nullptr)
        return s_pollOpTable[code](s_currentOp);

    return true;
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

    VERBOSE("Activities: '%s' -> '%s'\n", oldAct->name, newAct->name);

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
void invokeBindings(uint32_t protocol, uint64_t value, IrEventKind kind)
{
    if (!activitiesConfig || activitiesConfig->activities_count == 0) 
        return;

    activity* act = &activitiesConfig->activities[s_currentActivity];
    unsigned long now = millis();
    uint32_t kindMask = (uint32_t)kind;

    // Reset modifier if timed out
    if (s_modifierProtocol != 0 && now > s_modifierExpiry)
    {
        s_modifierProtocol = 0;
        s_modifierValue = 0;
        s_modifierExpiry = 0;
    }

    // Track hold time: record start on Press, clear on Release
    if (kind == IrEventKind::Press)
    {
        s_holdProtocol = protocol;
        s_holdValue    = value;
        s_holdStartMs  = now;
    }
    else if (kind == IrEventKind::Release)
    {
        s_holdProtocol = 0;
        s_holdValue    = 0;
        s_holdStartMs  = 0;
    }

    // Check bindings
    bool matchedAsModifier = false;
    bool matchedAny = false;
    for (uint32_t i = 0; i < act->bindings_count; i++)
    {
        binding* b = act->bindings[i];
        bool matched = false;
        switch (b->type)
        {
            case BINDING_TYPE_IR:
            {
                bindingIr* bir = (bindingIr*)b;

                // Correct protocol?
                if (bir->protocol != protocol)
                    break;
                if (bir->modifier && protocol != s_modifierProtocol)
                    break;

                // Correct modifier and value?
                if ((bir->eventMask & kindMask) != 0 &&
                    bir->value == value &&
                    bir->modifier == s_modifierValue)
                {
                    // Check minimum hold time
                    unsigned long held = (protocol == s_holdProtocol && value == s_holdValue)
                                        ? (now - s_holdStartMs) : 0;
                    if (held >= bir->minHoldTime)
                        matched = true;
                }
                else if (s_modifierProtocol == 0 && bir->modifier == value)
                {
                    // Modifier
                    matchedAsModifier = true;
                }
                break;
            }
            
            case BINDING_TYPE_IR_ANY:
                matched = !matchedAsModifier && (kind == IrEventKind::Press || kind == IrEventKind::Repeat);
                break;
        }

        if (matched)
        {
            matchedAny = true;

            //VERBOSE("Matched binding #%u\n", i);

            // On press, configure synthetic repeat rate if the binding specifies one
            if (kind == IrEventKind::Press && b->type == BINDING_TYPE_IR)
            {
                bindingIr* bir2 = (bindingIr*)b;
                if (bir2->repeatRate > 0)
                    setIrRepeatRate(bir2->repeatRate);
            }

            // Suppress release event
            if (kind == IrEventKind::LongPress)
            {
                s_holdProtocol = 0;
                s_holdValue    = 0;
                s_holdStartMs  = 0;
                suppressRelease();
            }

            // Enqueue op to set the ir code register
            setIrRegOp* sr = (setIrRegOp*)malloc(sizeof(setIrRegOp));
            if (!sr)
            {
                LOG("Activities: failed to alloc setIrRegOp\n");
                break;
            }
            sr->base.op  = OP_SET_IR_REG | OP_FLAG_HEAP;
            sr->protocol = protocol;
            sr->irCode   = value;
            enqueueOp((op*)sr);

            // Enqueue the binding's operations
            enqueueOps(b->ops, b->ops_count);

            // Quit routing?
            if (!(b->flags & BINDING_FLAGS_CONTINUE_ROUTING))
                break;
        }
    }

    // Update modifier state
    if (matchedAny)
    {
        // Clear modifier
        s_modifierProtocol = 0;
        s_modifierValue = 0;
        s_modifierExpiry = 0;
    }
    else if (matchedAsModifier)
    {
        // Set modifier
        s_modifierProtocol = protocol;
        s_modifierValue = value;
        s_modifierExpiry = now + MODIFIER_TIMEOUT_MS;
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

    VERBOSE("Activities: loaded %d device(s), %d activity(s)\n",
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
    VERBOSE("Activities: initial activity '%s'\n", act->name);
}

// ---- Public API ----

bool isActivitiesBusy()
{
    return !isQueueEmpty() || s_currentOp != nullptr;
}

void setupActivities()
{
    s_httpMutex = xSemaphoreCreateMutex();
    if (loadActivities())
        activateInitial();
}

bool reloadActivities()
{
    // Free any heap-allocated ops still in the queue before discarding it.
    while (!isQueueEmpty())
    {
        op* o = s_opQueue[s_opHead];
        s_opHead = (s_opHead + 1) % OP_QUEUE_SIZE;
        if (o->op & OP_FLAG_HEAP) free(o);
    }
    if (s_currentOp && (s_currentOp->op & OP_FLAG_HEAP))
        free(s_currentOp);

    // Reset all runtime state before reloading.
    s_opHead = s_opTail = 0;
    s_currentOp        = nullptr;
    s_deviceOnMask     = 0;
    s_currentActivity  = 0;
    s_modifierProtocol = 0;
    s_stringReg        = "";
    s_boolReg          = false;
    s_irReg            = {0, 0};

    xSemaphoreTake(s_httpMutex, portMAX_DELAY);
    s_httpDoneGen      = s_httpCurrentGen;  // discard any pending result
    s_httpTaskResponse = "";
    xSemaphoreGive(s_httpMutex);

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
        {
            if (s_currentOp->op & OP_FLAG_HEAP) free(s_currentOp);
            s_currentOp = nullptr;
        }
        return;  // at most one op per cycle
    }

    // Dequeue and start the next op.
    if (!isQueueEmpty())
    {
        op* next = s_opQueue[s_opHead];
        s_opHead = (s_opHead + 1) % OP_QUEUE_SIZE;
        startOp(next);
        // Free heap-allocated ops once they have finished executing.
        // Timed ops (delay, LED) set s_currentOp and are freed on completion above.
        if (s_currentOp == nullptr && (next->op & OP_FLAG_HEAP))
            free(next);
    }
}
