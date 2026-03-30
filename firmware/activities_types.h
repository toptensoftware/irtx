#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// op
#define OP_SEND_IR 1
#define OP_SEND_WOL 2
#define OP_HTTP_GET 3
#define OP_HTTP_POST 4
#define OP_UDP_PACKET 5
#define OP_DELAY 6
#define OP_LED 7
#define OP_SWITCH_ACTIVITY 8
#define OP_SET_IR_REG 9
#define OP_SEARCH_STRING 10
#define OP_IF_TRUE 11
#define OP_WAIT_HTTP 12

// binding_type
#define BINDING_TYPE_IR 1
#define BINDING_TYPE_IR_ANY 2
#define BINDING_TYPE_GPIO 3
#define BINDING_TYPE_GPIO_ENCODER 4

// binding_flags
#define BINDING_FLAGS_CONTINUE_ROUTING 1

// ir_event_kind_mask
#define IR_EVENT_KIND_MASK_PRESS 1
#define IR_EVENT_KIND_MASK_REPEAT 2
#define IR_EVENT_KIND_MASK_LONG_PRESS 4
#define IR_EVENT_KIND_MASK_RELEASE 8

// Forward declarations
typedef struct activitiesRoot activitiesRoot;
typedef struct device device;
typedef struct activity activity;
typedef struct op op;
typedef struct sendIrOp sendIrOp;
typedef struct sendWolOp sendWolOp;
typedef struct httpGetOp httpGetOp;
typedef struct httpPostOp httpPostOp;
typedef struct udpPacketOp udpPacketOp;
typedef struct delayOp delayOp;
typedef struct ledOp ledOp;
typedef struct switchActivityOp switchActivityOp;
typedef struct setIrRegOp setIrRegOp;
typedef struct searchStringOp searchStringOp;
typedef struct ifTrueOp ifTrueOp;
typedef struct waitHttpOp waitHttpOp;
typedef struct binding binding;
typedef struct bindingIr bindingIr;
typedef struct bindingIrAny bindingIrAny;
typedef struct bindingGpio bindingGpio;
typedef struct bindingGpioEncoder bindingGpioEncoder;


// activitiesRoot
struct __attribute__((packed)) activitiesRoot
{
	/*    0 */	int32_t version;
	/*    4 */	uint32_t devices_count;
	/*    8 */	device* devices;
	/*   12 */	uint32_t activities_count;
	/*   16 */	activity* activities;
};
static_assert(sizeof(activitiesRoot) == 20, "Size of activitiesRoot must be 20 bytes");

// device
struct __attribute__((packed)) device
{
	/*    0 */	const char* name;
	/*    4 */	uint32_t turnOn_count;
	/*    8 */	op** turnOn;
	/*   12 */	uint32_t turnOff_count;
	/*   16 */	op** turnOff;
};
static_assert(sizeof(device) == 20, "Size of device must be 20 bytes");

// activity
struct __attribute__((packed)) activity
{
	/*    0 */	const char* name;
	/*    4 */	uint32_t devices_count;
	/*    8 */	int32_t* devices;
	/*   12 */	uint32_t bindings_count;
	/*   16 */	binding** bindings;
	/*   20 */	uint32_t willActivate_count;
	/*   24 */	op** willActivate;
	/*   28 */	uint32_t didActivate_count;
	/*   32 */	op** didActivate;
	/*   36 */	uint32_t willDeactivate_count;
	/*   40 */	op** willDeactivate;
	/*   44 */	uint32_t didDeactivate_count;
	/*   48 */	op** didDeactivate;
};
static_assert(sizeof(activity) == 52, "Size of activity must be 52 bytes");

// op
struct __attribute__((packed)) op
{
	/*    0 */	uint32_t op;
};
static_assert(sizeof(op) == 4, "Size of op must be 4 bytes");

// sendIrOp
struct __attribute__((packed)) sendIrOp
{
	/*    0 */	op base;
	/*    4 */	uint32_t protocol;
	/*    8 */	uint64_t irCode;
	/*   16 */	uint32_t ipAddr;
	/*   20 */	uint32_t sendAsRepeat;
};
static_assert(sizeof(sendIrOp) == 24, "Size of sendIrOp must be 24 bytes");

// sendWolOp
struct __attribute__((packed)) sendWolOp
{
	/*    0 */	op base;
	/*    4 */	uint8_t macaddr[6];
	uint8_t _pad1[2];
};
static_assert(sizeof(sendWolOp) == 12, "Size of sendWolOp must be 12 bytes");

// httpGetOp
struct __attribute__((packed)) httpGetOp
{
	/*    0 */	op base;
	/*    4 */	const char* url;
};
static_assert(sizeof(httpGetOp) == 8, "Size of httpGetOp must be 8 bytes");

// httpPostOp
struct __attribute__((packed)) httpPostOp
{
	/*    0 */	op base;
	/*    4 */	const char* url;
	/*    8 */	uint32_t data_count;
	/*   12 */	uint8_t* data;
	/*   16 */	const char* contentType;
	/*   20 */	const char* contentEncoding;
};
static_assert(sizeof(httpPostOp) == 24, "Size of httpPostOp must be 24 bytes");

// udpPacketOp
struct __attribute__((packed)) udpPacketOp
{
	/*    0 */	op base;
	/*    4 */	uint32_t ipAddr;
	/*    8 */	uint32_t data_count;
	/*   12 */	uint8_t* data;
};
static_assert(sizeof(udpPacketOp) == 16, "Size of udpPacketOp must be 16 bytes");

// delayOp
struct __attribute__((packed)) delayOp
{
	/*    0 */	op base;
	/*    4 */	uint32_t duration;
};
static_assert(sizeof(delayOp) == 8, "Size of delayOp must be 8 bytes");

// ledOp
struct __attribute__((packed)) ledOp
{
	/*    0 */	op base;
	/*    4 */	uint32_t color;
	/*    8 */	uint32_t period;
	/*   12 */	uint32_t duration;
};
static_assert(sizeof(ledOp) == 16, "Size of ledOp must be 16 bytes");

// switchActivityOp
struct __attribute__((packed)) switchActivityOp
{
	/*    0 */	op base;
	/*    4 */	uint32_t index;
};
static_assert(sizeof(switchActivityOp) == 8, "Size of switchActivityOp must be 8 bytes");

// setIrRegOp
struct __attribute__((packed)) setIrRegOp
{
	/*    0 */	op base;
	/*    4 */	uint32_t protocol;
	/*    8 */	uint64_t irCode;
};
static_assert(sizeof(setIrRegOp) == 16, "Size of setIrRegOp must be 16 bytes");

// searchStringOp
struct __attribute__((packed)) searchStringOp
{
	/*    0 */	op base;
	/*    4 */	const char* matchString;
};
static_assert(sizeof(searchStringOp) == 8, "Size of searchStringOp must be 8 bytes");

// ifTrueOp
struct __attribute__((packed)) ifTrueOp
{
	/*    0 */	op base;
	/*    4 */	uint32_t trueOps_count;
	/*    8 */	op** trueOps;
	/*   12 */	uint32_t falseOps_count;
	/*   16 */	op** falseOps;
};
static_assert(sizeof(ifTrueOp) == 20, "Size of ifTrueOp must be 20 bytes");

// waitHttpOp
struct __attribute__((packed)) waitHttpOp
{
	/*    0 */	op base;
};
static_assert(sizeof(waitHttpOp) == 4, "Size of waitHttpOp must be 4 bytes");

// binding
struct __attribute__((packed)) binding
{
	/*    0 */	uint32_t type;
	/*    4 */	uint32_t flags;
	/*    8 */	uint32_t doOps_count;
	/*   12 */	op** doOps;
};
static_assert(sizeof(binding) == 16, "Size of binding must be 16 bytes");

// bindingIr
struct __attribute__((packed)) bindingIr
{
	/*    0 */	binding base;
	/*   16 */	uint32_t protocol;
	/*   20 */	uint32_t eventMask;
	/*   24 */	uint64_t modifier;
	/*   32 */	uint64_t value;
	/*   40 */	uint32_t minHoldTime;
	/*   44 */	uint32_t repeatRate;
};
static_assert(sizeof(bindingIr) == 48, "Size of bindingIr must be 48 bytes");

// bindingIrAny
struct __attribute__((packed)) bindingIrAny
{
	/*    0 */	binding base;
};
static_assert(sizeof(bindingIrAny) == 16, "Size of bindingIrAny must be 16 bytes");

// bindingGpio
struct __attribute__((packed)) bindingGpio
{
	/*    0 */	binding base;
	/*   16 */	uint32_t pin;
	/*   20 */	uint32_t eventMask;
	/*   24 */	uint32_t minHoldTime;
	/*   28 */	uint32_t initialDelay;
	/*   32 */	uint32_t repeatRate;
};
static_assert(sizeof(bindingGpio) == 36, "Size of bindingGpio must be 36 bytes");

// bindingGpioEncoder
struct __attribute__((packed)) bindingGpioEncoder
{
	/*    0 */	binding base;
	/*   16 */	uint32_t pin;
	/*   20 */	int32_t direction;
	/*   24 */	uint32_t minVelocityPeriod;
};
static_assert(sizeof(bindingGpioEncoder) == 28, "Size of bindingGpioEncoder must be 28 bytes");

#ifdef __cplusplus
}
#endif

