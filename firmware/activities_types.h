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

// binding_type
#define BINDING_TYPE_IR 1

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
typedef struct binding binding;
typedef struct bindingIr bindingIr;


// activitiesRoot
struct __attribute__((packed)) activitiesRoot
{
	/*    0 */	int32_t version;
	/*    4 */	uint32_t devices_count;
	/*    8 */	device* devices;
	/*   12 */	uint32_t activities_count;
	/*   16 */	activity* activities;
};

// device
struct __attribute__((packed)) device
{
	/*    0 */	const char* name;
	/*    4 */	uint32_t onOps_count;
	/*    8 */	op** onOps;
	/*   12 */	uint32_t offOps_count;
	/*   16 */	op** offOps;
};

// activity
struct __attribute__((packed)) activity
{
	/*    0 */	const char* name;
	/*    4 */	uint32_t devices_count;
	/*    8 */	int32_t* devices;
	/*   12 */	uint32_t bindings_count;
	/*   16 */	binding** bindings;
	/*   20 */	uint32_t willActivateOps_count;
	/*   24 */	op** willActivateOps;
	/*   28 */	uint32_t didActivateOps_count;
	/*   32 */	op** didActivateOps;
	/*   36 */	uint32_t willDeactivateOps_count;
	/*   40 */	op** willDeactivateOps;
	/*   44 */	uint32_t didDectivateOps_count;
	/*   48 */	op** didDectivateOps;
};

// op
struct __attribute__((packed)) op
{
	/*    0 */	uint32_t op;
};

// sendIrOp
struct __attribute__((packed)) sendIrOp
{
	/*    0 */	op base;
	/*    4 */	uint32_t protocol;
	/*    8 */	uint64_t irCode;
	/*   16 */	uint32_t ipAddr;
};

// sendWolOp
struct __attribute__((packed)) sendWolOp
{
	/*    0 */	op base;
	/*    4 */	uint8_t macaddr[6];
	uint8_t _pad1[2];
};

// httpGetOp
struct __attribute__((packed)) httpGetOp
{
	/*    0 */	op base;
	/*    4 */	const char* url;
};

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

// udpPacketOp
struct __attribute__((packed)) udpPacketOp
{
	/*    0 */	op base;
	/*    4 */	uint32_t ipAddr;
	/*    8 */	uint32_t data_count;
	/*   12 */	uint8_t* data;
};

// delayOp
struct __attribute__((packed)) delayOp
{
	/*    0 */	op base;
	/*    4 */	uint32_t duration;
};

// ledOp
struct __attribute__((packed)) ledOp
{
	/*    0 */	op base;
	/*    4 */	uint32_t color;
	/*    8 */	uint32_t period;
	/*   12 */	uint32_t duration;
};

// switchActivityOp
struct __attribute__((packed)) switchActivityOp
{
	/*    0 */	op base;
	/*    4 */	uint32_t index;
};

// binding
struct __attribute__((packed)) binding
{
	/*    0 */	uint32_t type;
};

// bindingIr
struct __attribute__((packed)) bindingIr
{
	/*    0 */	binding base;
	/*    4 */	uint32_t protocol;
	/*    8 */	uint64_t modifier;
	/*   16 */	uint64_t value;
	/*   24 */	uint32_t ops_count;
	/*   28 */	op** ops;
};

#ifdef __cplusplus
}
#endif

