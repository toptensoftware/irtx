# irtx

An open source flexible firmware for ESP32 based Infrared transmitter/receivers.

irtx can act as a dumb blaster:

* Receive IR commands over UDP and transmit using an attached IR transmitter circuit
* Accept either raw timing data, or NEC or Panasonic encoded values
* Pair with up to 4 BLE devices to send keyboard, mouse or consumer control HID packets

irtx can also act as a smart device listening for particular actions and invoking actions

* Decode and match received IR signals (supports NEC and Panasonic protocols)
* Handle input events from GPIO attached buttons and rotary encoders
* Invoke sequences of operations in response to IR or GPIO input actions
* Manage a set of "activities" and switch between them

Configuration by serial, telnet or web.

An accompanying NodeJS library [irtx-node](https://github.com/toptensoftware/irtx-node) can 
be used to programatically control the device (eg: send IR codes).  It also provides
a command line tool for sending IR codes and compiling and uploading activity definitions.


## Devices

* [`blaster`](./blaster) - a simple dumb IR blaster that can be placed inconspicously at the back of a room
  providing a way to blast IR from any Wifi enabled device.


## Building the Firmware

The firmware for the esp32 is an Arduino project.  To build

1. Install the `arduino-cli` tools

2. Install the board

    ```bash
    arduino-cli core update-index --additional-urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
    arduino-cli core install esp32:esp32 --additional-urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
    ```

3. Install libraries

    ```bash
    arduino-cli lib install "Adafruit NeoPixel"
    arduino-cli config set library.enable_unsafe_install true
    arduino-cli lib install --git-url https://github.com/h2zero/NimBLE-Arduino#nvs-custom-namespace
    #arduino-cli lib install "NimBLE-Arduino"
    ```

    (currently requires a dev branch build of NimBLE)

4. Build

    ```bash
    ./build --build c3
    ```

5. Flash (replace com8 with your serial port)

    ```bash
    ./build --flash c3 com8
    ```

Alternatively, you can build and flash in one command:

```
./build c3 com8
```

The firmware also supportes esp32-c6 by passing "c6" instead of "c3" to the build command.



## Activities

Besides acting as a dumb IR blaster, irtx can also manage a set of "activities". 

The activities system supports:

* A set of devices each with a list of operations for turning the device on and off
* A set of activities each of which defines 
    - a set of required devices (which will be turned on, others will be turned off) 
    - a set of operations to and invoke when the activity is switch to, and away from
    - a set of bindings that map input actions (GPIO or received IR commands) to a 
      sequence of operations.

Available operations include:

- Send an IR code
- Send a Wake on Lan packet
- Make an HTTP GET or POST request
- Send a UDP packet
- Delay
- Set the onboard LED
- Switch activities
- Search for a string in a HTTP GET response
- Perform a conditional set of instructions depending on string match

Unlike device configuration which is done via the console and stored in NVS on the device, activity
definitions are compiled from JSON like definitions and uploaded as binary packed data.

The activity definitions can edited either:

* directly in the web UI, or http://my-irtx/activity-editor
* compiled and uploaded using the [node-irtx](https://github.com/toptensoftware/irtx-node) command line tool.

eg: 
```
npx toptensoftware/irtx-node -h my-irtx activites my-activities.js
```

Here's an example activities configuration file (intended for illustration only)

```js
import { op } from "irtx:binpack";

// IR Codes
let ircodes = {
    tvOn: "NEC:0x20dfb34c",
    tvOff: "NEC:0x20dfa35c",
    volumeUp: "PANA:0x400401000405",
    volumeDown: "PANA:0x400401008485",
    ///etc...
}

// Base URL for Yamaha receiver
const yamaUrl = "http://10.1.1.125/YamahaExtendedControl/v1";

// Main Config
const confg = {
    version: 1,
    devices: [
        { 
            // TV Device
            name: "tv", 
            turnOn: op.sendIr(ircodes.tvOn),
            turnOff: op.sendIr(ircodes.tvOff)
        },
        {
            // Yamaha Receiver Device
            name: "receiver", 
            turnOn: op.httpGet(yamaUrl + "/main/setPower?power=on"),
            turnOff: op.httpGet(yamaUrl + "/main/setPower?power=standby"),
        }
    ],
    activities: [
        {
            name: "off",
            devices: [ /* no devices, turn everything off */ ],
        },
        {
            // Watch TV activity
            name: "watchTv",

            // Required devices
            devices: [ "tv", "receiver", ],

            // Actions to invoke when this activity is activated/deactivated
            //didDeactivate: [ ],
            didActivate: op.httpGet(yamaUrl + "/main/setInput?input=hdmi1"),
            //willActivate: [ ],
            //willDeactivate: [ ],

            // Bindings
            bindings: [
                {
                    // Volume up
                    on: ircodes.volumeUp,
                    do: op.httpGet(recUrl +"/main/setVolume?volume=up"),
                },
                {
                    // Volume down
                    on: ircodes.volumeDown,
                    do: op.httpGet(recUrl +"/main/setVolume?volume=down"),
                },
            ]
        },
        {
            // Activity 2 etc...
        }
    ]
}

// The root configuration must be the default export
export default config;

```


## Writing `activities.js`

Activity configurations are written as ES module JavaScript files.  They can be edited
directly in the built-in web UI at `http://<device>/activity-editor`, or compiled to binary
locally using the `@toptensoftware/binpack` tool (or the `irtx-node` CLI which wraps it).

### Importing helpers

The `binpack.js` schema file (served from the device at `/binpack.js` and mapped to the
module specifier `irtx:binpack` by the toolchain) exports everything you need:

```js
import { op, binding, repeatBehaviour, irEventKindMask, bindingFlags,
         riff, parseIPv4, parseMacAddress } from "irtx:binpack";
```

### File structure

An `activities.js` file must default-export the root configuration object:

```js
export default {
    version: 1,
    devices: [ /* device definitions */ ],
    activities: [ /* activity definitions */ ],
};
```

### Devices

Each device has a name and optional `turnOn` / `turnOff` op sequences.  When switching
activities, the firmware powers devices on and off automatically based on which devices the
old and new activities reference.

```js
devices: [
    {
        name: "tv",
        turnOn:  op.sendIr("NEC:0x20dfb34c"),
        turnOff: op.sendIr("NEC:0x20dfa35c"),
    },
    {
        name: "receiver",
        turnOn:  op.httpGet("http://10.1.1.125/YamahaExtendedControl/v1/main/setPower?power=on"),
        turnOff: op.httpGet("http://10.1.1.125/YamahaExtendedControl/v1/main/setPower?power=standby"),
    },
]
```

`turnOn` and `turnOff` accept either a single op or an array of ops.

### Activities

Each activity declares the devices it needs and optional lifecycle hooks:

```js
activities: [
    {
        name: "off",
        devices: [],               // all devices powered off
    },
    {
        name: "watchTv",
        devices: ["tv", "receiver"],   // referenced by name; others are powered off

        // Lifecycle hooks — single op or array of ops, all optional
        willActivate:   [ /* ops run before devices are powered */ ],
        didActivate:    op.httpGet("http://10.1.1.125/.../main/setInput?input=hdmi1"),
        willDeactivate: [ /* ops run before devices are powered down */ ],
        didDeactivate:  [ /* ops run after leaving this activity */ ],

        bindings: [ /* see below */ ],
    },
]
```

Lifecycle order when switching from A → B:
`A.willDeactivate` → `B.willActivate` → device power changes → `B.didActivate` → `A.didDeactivate`

### Bindings

A binding maps an input event (`on`) to a sequence of ops (`do`):

```js
bindings: [
    {
        on: binding.ir("NEC:0x20df40bf"),         // trigger
        do: op.httpGet("http://...?volume=up"),   // single op or array of ops
    },
]
```

The optional `flags` field accepts values from `bindingFlags`:

| Flag | Meaning |
|------|---------|
| `bindingFlags.continue_routing` | Don't stop after this binding fires; continue checking remaining bindings |

#### `binding.ir(code)`

Matches a received IR code.  The `code` string format is:

- `"PROTOCOL:VALUE"` — e.g. `"NEC:0x20df40bf"`, `"PANA:0x400401000405"`
- `"PROTOCOL:MODIFIER+VALUE"` — fires only when `MODIFIER` was received within the last 5 s
- `"*"` — wildcard, matches any received IR code

The returned object can be extended with extra fields:

| Field | Default | Description |
|-------|---------|-------------|
| `eventMask` | `press` | Bitmask of `irEventKindMask` values to match |
| `minHoldTime` | `0` | Minimum hold time in ms before the binding fires (0 = immediate) |
| `repeatRate` | `0` | Synthetic repeat interval in ms (0 = natural IR repeat rate) |

```js
{ on: { ...binding.ir("NEC:0x20df40bf"), eventMask: irEventKindMask.press | irEventKindMask.repeat }, do: ... }
```

#### `binding.gpio(pin)`

Matches a button press on the given GPIO pin number (configured with `gpio <pin> pulldown/pullup`).

Extra fields available on the returned object:

| Field | Default | Description |
|-------|---------|-------------|
| `eventMask` | `press` | `irEventKindMask.press`, `.repeat`, `.release` |
| `minHoldTime` | `0` | ms held before firing (0 = immediate) |
| `initialDelay` | `0` | ms after press before first repeat (0 = use repeatRate) |
| `repeatRate` | `0` | Repeat interval in ms (0 = no repeat) |

#### `binding.encoder_inc(pin)` / `binding.encoder_dec(pin)`

Matches rotary encoder clockwise (`_inc`) or counter-clockwise (`_dec`) rotation on the
encoder whose A-pin is `pin`.  The optional `minVelocityPeriod` field (ms, default 0) limits
firing to encoder velocities at or below the given period.

### Operations (`op`)

All ops can be used as a single value or inside an array wherever an op sequence is expected.

#### `op.sendIr(irCode, ipAddr?, repeat?)`

Transmits an IR code locally or forwards it to a remote irtx device via UDP.

- `irCode` — `"PROTOCOL:VALUE"` string, e.g. `"NEC:0x20df40bf"`
- `ipAddr` — IPv4 address string `"10.1.1.x"` or integer (default `0` = transmit locally)
- `repeat` — one of the `repeatBehaviour` constants (default `repeatBehaviour.default`)

`repeatBehaviour` values:

| Constant | Behaviour |
|----------|-----------|
| `repeatBehaviour.default` | Block the op queue until the IR transmitter is free, then send a full code frame |
| `repeatBehaviour.sendRepeat` | If the same code is already in-flight, send a NEC repeat frame and unblock; otherwise block |
| `repeatBehaviour.dropIfBusy` | If the same code is already in-flight, discard and unblock; otherwise block |

#### `op.httpGet(url)`

Makes an HTTP GET request to `url`.

#### `op.httpPost(url)`

Makes an HTTP POST request to `url`.

#### `op.sendWol(macaddr)`

Sends a Wake-on-LAN magic packet to the given MAC address (`"AA:BB:CC:DD:EE:FF"` string or
array of 6 bytes).

#### `op.delay(ms)`

Pauses op-queue execution for `ms` milliseconds.

#### `op.led(color)`

Sets the onboard LED to `color` (a packed 32-bit RGB integer).  The optional `period` and
`duration` fields (set directly on the returned object) control blink period and auto-off time.

#### `op.udp(ip, data)`

Sends a raw UDP packet.  `ip` is an IPv4 address string or integer; `data` is a byte array.

#### `op.switchActivity(indexOrName)`

Switches to another activity, referenced by its zero-based index or by name string.

#### `op.match_string(str)`

Searches the body of the most recent HTTP response for `str`.  Sets an internal boolean
register used by `op.if_true`.

#### `op.if_true(trueOps, falseOps)`

Branches on the result of the most recent `op.match_string` call.  `trueOps` and `falseOps`
are each a single op or an array of ops.

#### `op.wait_http()`

Blocks the op queue until the most recent HTTP request completes.

### Helper functions

#### `riff(str)`

Encodes a 4-character ASCII string as a little-endian 32-bit FourCC integer (the format used
for IR protocol IDs).  Pass a string (`"NEC "`, `"PANA"`) or an integer (returned unchanged).

#### `parseIPv4(ip)`

Parses a dotted-decimal IPv4 address string into a 32-bit integer.  Pass a string or integer
(returned unchanged).

#### `parseMacAddress(mac)`

Parses a colon-separated MAC address string into an array of 6 integers.  Pass a string or
array (returned unchanged).

### Constants

| Export | Values |
|--------|--------|
| `irEventKindMask` | `press` (1), `repeat` (2), `long_press` (4), `release` (8) |
| `repeatBehaviour` | `default` (0), `sendRepeat` (1), `dropIfBusy` (2) |
| `bindingFlags` | `continue_routing` (1) |
| `bindingType` | `ir` (1), `ir_any` (2), `gpio` (3), `gpio_encoder` (4) |
| `opId` | Numeric IDs for all op types (normally accessed via the `op` class helpers) |


## Configuring the Device

Wifi needs to be configured through serial terminal program.  Once Wifi is enabled, all other
configuration can be done through serial, telnet or via the built-in web based terminal.

To setup Wifi, use a serial monitor program (eg: [ttsm](https://github.com/toptensoftware/ttsm)) to configure the device.  Use the `setwifi` command to setup connection to your Wifi network.

Alternatively you can configure the device to act as a Wifi Access Point using the `setap` command, 
however since AP mode can only be entered using a boot pin configuration you also need a physical 
button on the device and it needs to be configured using the `gpio` and `setbootpin` commands.

Once Wifi or AP mode is active, you can use telnet or the built-in web UI to configure the rest
of the device using the console commands described below.

## Console Commands

The following console commands are available via serial, telnet or using the web UI.

 *   `name <devicename>`                  - set device name, takes effect after restart
 *   `setwifi <ssid> <password>`          - configure WiFi credentials and reconnect
 *   `setap <ssid> [<password>]`          - configure access point credentials (default password: `irtx`)
 *   `setbootpin <pin> [<pin>]`           - configure pin(s) that trigger AP mode at boot (see below)
 *   `status`                             - show device name, MAC, WiFi, IP, BLE pairing status
 *   `pair <bleslot>`                     - pair a BLE device with a specified slot
 *   `unpair <bleslot>`                   - unpair a BLE device slot
 *   `connect <bleslot>`                  - connect to the paired device for a slot
 *   `led <r> <g> <b>` / `led clear`     - set or clear the user LED colour
 *   `activity <name|index>`              - switch to an activity by name or index
 *   `setdefact <index>`                  - set the default activity loaded on boot (default: 0)
 *   `gpio [<pin> [<pin>] <mode>]`        - show or configure GPIO pins
 *   `verbose on|off`                     - enable or disable verbose logging (persisted across reboots)
 *   `nvsreset`                           - factory reset
 *   `nvsdump`                            - dump keys in NVS
 *   `reboot`                             - reboot the device
 *   `dmesg`                              - displays the message log (up to 50 past logged messages)
 *   `help`                               - list all commands

## Configuring GPIO Pins

By default, no GPIO pins are configured.

To set the status LED neopixel to GRB format on pin 10 (eg: Waveshare C3 zero dev board)

```
gpio 10 grb
```

To set the status LED neopixel to GRB format on pin 8 (eg: Waveshare C6 WROOM dev board)

```
gpio 8 grb
```

To set the pins the IR receiver (eg pin 5) and transmitter (eg pin 4) are connected:

```
gpio 5 irrx
gpio 4 irtx
```

To set a generic button input with pull down:

```
gpio 11 pulldown
```

To set a rotary encoder input, pass the A/B pin pair:

```
gpio 12 13 pulldown
```

Note: button and encoder inputs don't do anything unless also configured in the activities 
configuration to perform operations.


## Access Point Mode

The device can be started in WiFi access point (AP) mode for configuration via telnet or web without
needing to connect to an existing network.

1. Configure the AP SSID (and optionally a password, default is `irtx1234`):

    ```
    setap mydevice-ap
    ```

2. Configure a GPIO pin that will trigger AP mode when held at boot. The pin should also be configured
   as a pullup or pulldown input using the `gpio` command:

    ```
    gpio 9 pullup
    setbootpin 9
    ```

3. Press the attached button while powering on the device. The status LED (if configured) will turn 
   yellow to confirm AP mode is active.

4. On your computer or phone, connect to the WiFi network matching the AP SSID you configured,
   using the AP password.

5. Either: 
    * telnet to `192.168.4.1` to access the device terminal
    * visit `http://192.168.4.1` and use the console tab 

Two pins can be specified with `setbootpin` — both must be pressed simultaneously at boot to trigger
AP mode (useful to avoid accidental triggers).



## Telnet Support

Once wifi is enabled and working you can use telnet (eg: [ttsm](https://github.com/toptensoftware/ttsm)) to configure and monitor the device using the same commands as above.

Note: only one telnet client can connect at a time.



## Pairing BLE Devices

Up to 4 paired BLE devices are supported, but only one can be active at a time.  To pair a device:

1. Connect using a serial monitor program or telnet
2. Type `pair N` and the device will start advertising itself for pairing (eg: type `pair 0` for the first device)
3. Go to the device to be paired and choose to pair it.
4. You should see connected and authenicated messages in the terminal and then a message 
   saying the device has been paired.
5. Pair subsequent devices by typing `pair 1`, `pair 2` and `pair 3`

To test connection, use the `connect N` command.   The paired device should automatically reconnect if available.  Use
`connect -1` to disconnect all devices.  Once all devices are paired it's recommended to test switching the connection
between all devices to ensure everything is setup correctly.

Use the `unpair N` command to unpair a device - you should also unpair from the other device too. (eg: "forget this device", "remove device")

The `status` command shows the public addresses for each of the virtual pairing slots, the address of any paired device and the
connection status.

Note: BLE can be a bit flakey during pairing.  Sometimes you might need to toggle bluetooth on/off on the device being paired
to get it to initially connect.



## UDP Protocol

The following describes the UDP protocol for communicating with the device.

Node.js clients can use the [irtx-node](https://github.com/toptensoftware/irtx-node) library.

### IR Transmit

To transmit an IR signal, send a UDP packet to port 4210 in the following format:

* `uint16_t cmd` - must be 1
* `uint16_t reserved` - unused (used to be device index)
* `uint32_t carrierFreq` - carrier frequency
* `uint32_t gap` - a trailing gap (see below)
* `uint16_t timingData[]` - IR code timing values

(all values are little endian)

The `cmd` value must be 1 and indicates this is an IR transmission packet

The `carrierFrequency` must be `38000` otherwise the packet is ignored (might add support for others later)

The `gap` value is a minimum time (in microseconds) that the 
device will enforce between the end of one transmission and the start of the next.

The `timingData` is an array of microsecond timing values where event indices are pulses and odd indices are spaces.  If an odd number of values is passed an implicit zero gap is appended.

Note: pronto IR code definitions usually include the `gap` as the last space in the timing data. I recommended to pass that last space timing value as the `gap` parameter and set the trailing timing space to 0.



### Connect BLE Device

Before sending HID packets to a BLE device, the device first needs to be connected:

* `uint16_t cmd` - must be 2
* `uint8_t bleslot` - the BLE virtual device slot to connect

Currently there's no network method to check if the connection succeeded.  Use the serial or telnet monitor to check status manually.



### Send BLE HID Input Report

Once a BLE device is connected, HID reports can be sent

* `uint16_t cmd` - must be 3
* `uint8_t bleslot` - the BLE virtual device slot to connect
* `uint8_t reportId` - the kind of hid report to send
* `uint8_t reportData[]` - the actual HID report

The `reportId` and `reportData[]` should be

* `reportId` = 1, `reportData[8]` - keyboard input report
* `reportId` = 2, `reportData[2]` - consumer control report
* `reportId` = 3, `reportData[4]` - mouse input report

The format of the `reportData` is as per the BLE HID spec.  The length of the report data must be correct
otherwise the packet will be dropped (check serial/telnet monitor if packets are not being delivered).



### Transmit IR Code

To transmit an IR signal using a named protocol, send a UDP packet to port 4210 in the following format:

* `uint16_t cmd` - must be 4
* `uint16_t reserved` - unused (used to be device index)
* `uint32_t protocolId` - identifies the IR protocol (see below)
* `uint64_t code` - the IR code value to transmit
* `uint8_t repeat` - if non-zero, sends the protocol's repeat frame instead of a full code frame

(all values are little endian)

The device encodes `code` according to the named protocol and transmits it.  The trailing gap is determined by the protocol definition.

Supported protocol IDs are 4-character RIFF-style codes:

| Protocol   | ID bytes (`protocolId` little-endian) | Bits |
|------------|---------------------------------------|------|
| NEC        | `4E 45 43 20` (`0x2043454E`)          | 32   |
| Panasonic  | `50 41 4E 41` (`0x414E4150`)          | 48   |

If `repeat` is non-zero and the protocol defines a repeat frame, the repeat frame is sent instead of encoding `code`.  NEC defines a repeat frame; Panasonic does not.



### Switch Activity

Switches the current activity:

* `uint16_t cmd` - must be 5
* `uint32_t activityIndex` - the index of the activity to switch to


### Simulate IR Code

Simulates the receipt of an IR code and dispatches it through the activity system:

* `uint16 cmd` - must be 6
* `uint32 protocol` - riff encoded IR protocol
* `uint64 code` - the IR code
* `uint32` - the event kind  (press, release, see [irEventKindMask](./binpack.js))



## HTTP API

The device runs an HTTP server on port 80. All API endpoints return `text/plain` unless noted.

### `GET /api/status`

Returns a JSON object with the current device status (name, MAC address, WiFi, IP, BLE pairing state). Equivalent to the `status` serial/telnet command.

### `GET /api/dmesg`

Returns the device message log as plain text (up to the last 50 entries). Equivalent to the `dmesg` serial/telnet command.

### `POST /api/command`

Executes a serial/telnet command on the device. The request body is the command string (plain text). Returns the command output as plain text.

```
POST /api/command
Content-Type: text/plain

status
```

### `POST /api/upload?filename=<name>`

Uploads a file and writes it to LittleFS. The file is supplied as a `multipart/form-data` body with a single `file` field. The `filename` query parameter sets the destination path on the device — a leading `/` is added automatically if omitted.

```
POST /api/upload?filename=activities.bin
Content-Type: multipart/form-data; boundary=...
```

### `POST /api/reload-activities`

Reloads `activities.bin` from LittleFS and makes it the active configuration. Call this after uploading a new `activities.bin` via `/api/upload`.

### `GET /<path>`

Serves files in the following priority order:

1. **Built-in web UI** — files compiled into the firmware (gzip-compressed, served from flash)
2. **LittleFS** — user-uploaded files (e.g. `activities.bin`, `activities.js`)
3. **SPA fallback** — `GET`/`HEAD` requests that match no file return `index.html` so the client-side router can handle the URL

MIME types are inferred from the file extension: `.html`, `.css`, `.js`, `.json`, `.txt`, `.xml`, `.bin`, `.png`, `.jpg`/`.jpeg`, `.gif`, `.svg`, `.ico`, `.pdf`, `.gz`, `.zip`. Unknown extensions are served as `application/octet-stream`.

The following files have special meaning:

| File | Location | Description |
|------|----------|-------------|
| `/activities.js` | LittleFS | Source file for the current activity configuration (uploaded for reference) |
| `/activities.bin` | LittleFS | Compiled binary form of `activities.js`, loaded by the firmware at boot and on reload |
| `/binpack.js` | firmware (built-in) | Type definitions used to compile `activities.js` into `activities.bin` (fetched by the CLI at upload time) |

`/activities.js` and `/activities.bin` are kept in sync by the upload workflow — the CLI and web UI both upload both files together and then call `/api/reload-activities`.





## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.