# Project Notes

## Activities System

### What it is

The activities system lets the device behave differently depending on what the
user is doing — e.g. "watching TV", "playing games", "off". Each **activity**
has a name, a set of managed devices, a set of input bindings, and lifecycle
hooks. Only one activity is active at a time.

### Key concepts

**Devices** are external pieces of equipment (TV, receiver, etc.). Each device
has an `onOps` and `offOps` sequence. When switching activities the firmware
automatically powers devices on or off as needed, based on which devices the
old and new activities reference.

**Bindings** map an input event to a sequence of operations. Currently the only
binding type is `bindingIr`, which triggers on a received IR code
(`protocol` + `value`). Bindings support an optional `modifier` — a second IR
code that must have been received within 5 seconds for the binding to fire.
Bindings use a polymorphic type system (see below) so other trigger types
(BLE, HID, etc.) can be added in future without changing the activity struct.

**Operations** (ops) are the actions executed when a binding fires or a
lifecycle hook runs:

| Op | Description |
|----|-------------|
| `sendIrOp` | Transmit an IR code locally or forward it to a remote irtx device via UDP |
| `sendWolOp` | Send a Wake-on-LAN magic packet |
| `httpGetOp` / `httpPostOp` | Make an HTTP request |
| `udpPacketOp` | Send a raw UDP packet |
| `delayOp` | Pause execution for N ms |
| `ledOp` | Set the LED colour, with optional blink and duration |
| `switchActivityOp` | Switch to another activity by index |

Ops are executed asynchronously from a 32-entry ring buffer; one op is
processed per `pollActivities()` cycle. Timed ops (`delayOp`, `ledOp`) keep
the queue paused until they complete.

**Lifecycle hooks** run ops at transition points:
`willDeactivate` → `willActivate` → device on/off → `didActivate` → `didDeactivate`.
The activity index is committed only after all transition ops have executed.

### How it works at runtime

1. On boot, `setupActivities()` loads `activities.bin` from LittleFS, relocates
   internal pointers, and activates activity 0.
2. The main loop calls `pollActivities()` each cycle to drain the op queue.
3. When an IR code is received, `invokeBindings()` is called. It searches
   the current activity's binding list for a match and enqueues the bound ops.
4. `switchActivity(index)` can be called externally (UDP cmd=5) or from a
   `switchActivityOp` inside a sequence.

---

## Binpack — packing activities into binary data

User-defined activities are authored as a JavaScript object (`activities.js`)
and compiled to a flat binary (`activities.bin`) that the MCU can load directly
into RAM and cast to C structs.

### Schema (`binpack.js`)

`binpack.js` exports a list of type definitions consumed by the
`@toptensoftware/binpack` tool. Each type lists its fields with a name and a
primitive type:

| Type token | Meaning |
|------------|---------|
| `"int"` / `"uint"` | 32-bit signed / unsigned integer |
| `"ulong"` | 64-bit unsigned integer |
| `"string"` | Inline null-terminated string (pointer after relocation) |
| `"byte[N]"` | Fixed-length byte array |
| `"byte*"` | Counted byte array (preceded by a `"length"` field) |
| `"Foo*"` | Inline array of `Foo` structs |
| `"Foo**"` | Array of pointers to `Foo` (used for polymorphic types) |
| `"length"` | Emits the count for the following array field |

**Abstract / polymorphic types** work like C++ base classes. A type with
`resolveAbstractType` is abstract — binpack uses a discriminator field to
decide which concrete subtype to serialize. Subtypes declare `baseType` and
their fields are appended after the base fields. Two polymorphic hierarchies
exist:

- `op` (discriminator: `op` field) → `sendIrOp`, `sendWolOp`, `httpGetOp`,
  `httpPostOp`, `udpPacketOp`, `delayOp`, `ledOp`, `switchActivityOp`
- `binding` (discriminator: `type` field) → `bindingIr`

Because subtypes can have different sizes, they are always stored via pointer
arrays (`op**`, `binding**`) rather than inline arrays.

### Build pipeline

```
activities.js  ──(binpack)──►  activities.bin  ──(upload)──►  MCU LittleFS
binpack.js     ──(binpack)──►  firmware/activities_types.h
```

Pack `activities.bin` and upload to the device:

```
npx toptensoftware/binpack --pack:activities.bin activities.js
```

### Regenerating the C++ header

After modifying `binpack.js`, regenerate `firmware/activities_types.h` with:

```
npx toptensoftware/binpack --header:firmware/activities_types.h
```

Run from the project root.
