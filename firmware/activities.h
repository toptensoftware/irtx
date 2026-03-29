#pragma once

#include "activities_types.h"
#include "ir_rx.h"

#define MAX_ACTIVITIES_DEVICES 32

extern activitiesRoot* activitiesConfig;

// Lifecycle
void setupActivities();
void pollActivities();
bool reloadActivities();
bool isActivitiesBusy();

// Enqueue an array of op pointers into the execution ring buffer.
void enqueueOps(op** ops, uint32_t count);

// Switch to the given activity index.
// No-op if already current. Logs and ignores invalid indices.
// Rejected (logged) if the op queue is not empty.
void switchActivity(int index);

// Called by the IR RX path on every decoded code to check active bindings.
void invokeBindings(uint32_t protocol, uint64_t value, IrEventKind kind);

// Called by the GPIO poll path when a button changes state.
void invokeGpioBindings(int pin, bool pressed);

// Called by the GPIO poll path on every encoder detent.
void invokeEncoderBindings(int pin, int direction, uint32_t velocity);

// Print activities/device status to the console.
void statusActivities();
