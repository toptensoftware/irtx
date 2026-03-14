#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
typedef struct activitiesRoot activitiesRoot;
typedef struct device device;
typedef struct activity activity;
typedef struct command command;
typedef struct sendIrCommand sendIrCommand;
typedef struct sendWolCommand sendWolCommand;
typedef struct httpGetCommand httpGetCommand;
typedef struct httpPostCommand httpPostCommand;
typedef struct udpPacketCommand udpPacketCommand;
typedef struct delayCommand delayCommand;
typedef struct ledCommand ledCommand;
typedef struct switchActivityCommand switchActivityCommand;
typedef struct binding binding;
typedef struct deviceState deviceState;


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
	/*    4 */	uint32_t onCommands_count;
	/*    8 */	command** onCommands;
	/*   12 */	uint32_t offCommands_count;
	/*   16 */	command** offCommands;
};
static_assert(sizeof(device) == 20, "Size of device must be 20 bytes");

// activity
struct __attribute__((packed)) activity
{
	/*    0 */	const char* name;
	/*    4 */	uint32_t deviceStates_count;
	/*    8 */	deviceState* deviceStates;
	/*   12 */	uint32_t bindings_count;
	/*   16 */	binding* bindings;
	/*   20 */	uint32_t willActivateCommands_count;
	/*   24 */	command** willActivateCommands;
	/*   28 */	uint32_t didActivateCommands_count;
	/*   32 */	command** didActivateCommands;
	/*   36 */	uint32_t willDeactivateCommands_count;
	/*   40 */	command** willDeactivateCommands;
	/*   44 */	uint32_t didDectivateCommands_count;
	/*   48 */	command** didDectivateCommands;
};
static_assert(sizeof(activity) == 52, "Size of activity must be 52 bytes");

// command
struct __attribute__((packed)) command
{
	/*    0 */	uint32_t op;
};
static_assert(sizeof(command) == 4, "Size of command must be 4 bytes");

// sendIrCommand
struct __attribute__((packed)) sendIrCommand
{
	/*    0 */	command base;
	/*    4 */	uint32_t protocol;
	/*    8 */	uint64_t irCode;
	/*   16 */	uint32_t ipAddr;
};
static_assert(sizeof(sendIrCommand) == 20, "Size of sendIrCommand must be 20 bytes");

// sendWolCommand
struct __attribute__((packed)) sendWolCommand
{
	/*    0 */	command base;
	/*    4 */	uint8_t macaddr[6];
	uint8_t _pad1[2];
};
static_assert(sizeof(sendWolCommand) == 12, "Size of sendWolCommand must be 12 bytes");

// httpGetCommand
struct __attribute__((packed)) httpGetCommand
{
	/*    0 */	command base;
	/*    4 */	const char* url;
};
static_assert(sizeof(httpGetCommand) == 8, "Size of httpGetCommand must be 8 bytes");

// httpPostCommand
struct __attribute__((packed)) httpPostCommand
{
	/*    0 */	command base;
	/*    4 */	const char* url;
	/*    8 */	const char* data;
	/*   12 */	const char* contentType;
};
static_assert(sizeof(httpPostCommand) == 16, "Size of httpPostCommand must be 16 bytes");

// udpPacketCommand
struct __attribute__((packed)) udpPacketCommand
{
	/*    0 */	command base;
	/*    4 */	uint32_t ipAddr;
	/*    8 */	uint32_t data_count;
	/*   12 */	uint8_t* data;
};
static_assert(sizeof(udpPacketCommand) == 16, "Size of udpPacketCommand must be 16 bytes");

// delayCommand
struct __attribute__((packed)) delayCommand
{
	/*    0 */	command base;
	/*    4 */	uint32_t duration;
};
static_assert(sizeof(delayCommand) == 8, "Size of delayCommand must be 8 bytes");

// ledCommand
struct __attribute__((packed)) ledCommand
{
	/*    0 */	command base;
	/*    4 */	uint32_t color;
	/*    8 */	uint32_t period;
	/*   12 */	uint32_t duration;
};
static_assert(sizeof(ledCommand) == 16, "Size of ledCommand must be 16 bytes");

// switchActivityCommand
struct __attribute__((packed)) switchActivityCommand
{
	/*    0 */	command base;
	/*    4 */	uint32_t index;
};
static_assert(sizeof(switchActivityCommand) == 8, "Size of switchActivityCommand must be 8 bytes");

// binding
struct __attribute__((packed)) binding
{
	/*    0 */	uint32_t protocol;
	/*    4 */	uint64_t modifier;
	/*   12 */	uint64_t value;
	/*   20 */	uint32_t commands_count;
	/*   24 */	command** commands;
};
static_assert(sizeof(binding) == 28, "Size of binding must be 28 bytes");

// deviceState
struct __attribute__((packed)) deviceState
{
	/*    0 */	int32_t index;
	/*    4 */	bool on;
	uint8_t _pad1[3];
};
static_assert(sizeof(deviceState) == 8, "Size of deviceState must be 8 bytes");

#ifdef __cplusplus
}
#endif

