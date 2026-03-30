# irtx

An open source, Wifi enabled IR Blaster, IR Router and BLE HID Blaster.

Features:

* Receives IR commands over UDP and transmits using attached IR transmitter
* Accepts either raw timing data, or NEC or Panasonic encoded values
* Can receive and decode IR signals, remap and re-transmit either locally, or send via UDP for retransmission
  on a second irtx device or other UDP host
* Can be paired with up to 4 BLE devices to send keyboard, mouse or consumer control HID packets
* A NodeJS library [irtx-node](https://github.com/toptensoftware/irtx-node) can be used to control the device
* Configuration and monitoring via serial or telnet

![Final](./photos/final.jpeg)


## Build the Circuit

The IR transmitter circuit is easy to build on a small piece of veroboard:

Parts:

* C1 - 100nf ceramic cap
* C2 - 47uF electrolytic cap
* R1 - 680R
* R2 - 22R (or 10R, or 5R)
* BC337 NPN transistor
* 2x IR LEDs (like [these](https://www.jaycar.com.au/5mm-infrared-transmitting-led/p/ZD1945))
* Veroboard 13x8 pins
* [Waveshare ESP32-C3 Zero](https://www.waveshare.com/wiki/ESP32-C3-Zero)

![Circuit Diagram](./ircircuit/irtx-circuit.png)

![Veroboard](./ircircuit/irtx-board.png)

Notes: 

* the above diagram shows a jumper J1 - it's just for labelling where to solder the wires that go to the ESP32.
* don't forget the two cut tracks indicated by red X
* if I was to rebuild this I'd probably move the red jumper wire at the left to just to the right of the LEDs.
* The leads to C2 need to be bent and the cap "laid down" to fit in the case (see photo below)
* The IR LEDs leads need to be bent so they face out the left side of the diagram. (see photo below)
* Leave the veroboard slightly oversized and file down later to fit the snap-in clips in the case.

![Inner](./photos/inner.jpeg)



## Print the Case

This case supports IR transmitter circuitry only.  A version with IR receiver is not yet available.

![case](./case/case.png)

The case consists of three parts:

* [Top](./case/irtxcase-top.3mf)
* [Bottom](./case/irtxcase-bottom.3mf)
* [Clamp](./case/irtxcase-clamp.3mf)

You'll need a well calibrated printer to get the boards to clip in and the case to snap closed nicely.

The clamp is designed for a 15mm pole and requires:

* two M2.5 heat set inserts 
* two M2.5 6mm screws.

Or, design your own clamp for whereever you want to mount it to. The mounting holds are 26mm center-to-center. 

Fusion and 3mf files are the in the [./case](./case) subdirectory.

I printed the case in PLA, and the clamp in PETG.  You might be able to print it in ABS but I found the board clips too fragile.



## Connect the ESP32

There are three wires from the ESP32 Zero to the IR transmitter circuit.  

* The IR transmitter connection points are shown in the above stripboard image.

* The ESP32 Zero connections are to 5V and GND at the top left and signal to GPIO 4:

    ![ESP32-C3-Zero](./ircircuit/ESP32-C3-ZERO-Waveshare-pinout-high.jpg)




## Build the Firmware

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

4. Build

    ```bash
    cd firmware
    ./build --build c3
    ```

5. Flash (replace com8 with your serial port)

    ```bash
    ./build --flash c3 com8
    ```


The firmware also supportes esp32-c6 by passing "c6" instead of "c3" to the build command.  N

## Configuring the Device

Use a serial monitor program to configure the device.  The following serial terminal commands are available:

 *   `name <devicename>`          - set device name, takes effect after restart
 *   `setwifi <ssid> <password>`  - configure WiFi
 *   `status`                     - show device name, MAC, WiFi, IP, BLE pairing status
 *   `pair <bleslot>`             - pair a BLE device with a specified slot
 *   `unpair <bleslot>`           - unpair a BLE device slot
 *   `connect <bleslot>`          - connect to the paired device for a slot
 *   `nvsreset`                   - factory reset
 *   `nvsdump`                    - dump keys in NVS
 *   `reboot`                     - reboot the device
 *   `dmesg`                      - displays the message log (up to 50 past logged messages)
 *   `verbose on|off`             - enable or disable verbose logging (persisted across reboots)
 *   `gpio <pin> <function>`      - configures GPIO pins

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

Note: button and encoder inputs don't do anything unless also configured in the activities configuration to
perform operations.



## Telnet Support

Once wifi is enabled and working you can also use telnet to configure and monitor the device with the
same commands as above.

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

## Notes on IR Remapping

The device can receive NEC and Panasonic IR transmissions, map them to a different IR protocol/code and then either
re-transmit locally from the same device, or send the remapped code to a second irtx device for transmission.

Care needs to be taken when using this in the same room since overlapping IR codes will typically fail if a device
receives IR transmissions from two devices at the same time.

There are severak use cases for this:

1. In the same room by blocking a device's IR receiver with a hood containing an IR LED connected to an irtx device.  This blocks the 
  device from seeing IR from a remote while still allowing the irtx device to control it.

    This allows IR signals to a device to be intercepted, remapped and either passed through or used for secondary functions.  
  
    eg: with appropriate controlling software this could be used to convert a regular DVR remote into a universal remote for a home theatre.

2. Transmitting received IR codes to a separate room.

3. Transmitting received IR codes to other non-irtx machine.



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



### Set IR Route Table

The device can receive IR signals, look them up in a route table, and re-transmit them — either locally or forwarded to another irtx device over UDP.  The route table is set by sending a UDP packet to port 4210 in the following format:

* `uint16_t cmd` - must be 5
* `uint16_t count` - number of route entries that follow
* Route entries, repeated `count` times (28 bytes each, all fields little-endian):
  * `uint32_t srcProtocol` - protocol ID of the incoming IR code to match
  * `uint64_t srcCode` - IR code value to match
  * `uint32_t dstProtocol` - protocol ID to use for retransmission (0 = suppress)
  * `uint64_t dstCode` - IR code value to retransmit
  * `uint32_t dstIp` - destination IP address (0 = local retransmit)

This packet **replaces the entire route table** — send `count=0` to clear all routes.

The maximum number of routes is 64.

When an IR signal is received and decoded, its protocol and code are looked up against the table:

* **`dstProtocol == 0`** — suppress: the signal is silently dropped.
* **`dstIp == 0`** — local retransmit: the device transmits `dstProtocol:dstCode` via its own IR transmitter.
* **`dstIp != 0`** — remote forward: the device sends a cmd=4 UDP packet to the specified IP address on port 4210, which causes the remote irtx device to transmit `dstProtocol:dstCode`.

`dstIp` is stored little-endian with the first octet in the least-significant byte.  For example, `192.168.1.100` is encoded as `0x6401A8C0`.





## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.