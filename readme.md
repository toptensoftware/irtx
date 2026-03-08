# irtx

A simple, open source, Wifi enabled infrared transmitter (ie: an "IR Blaster") and BLE HID transmitter

![Final](./photos/final.jpeg)

As a BLE blaster the device appears as a keyboard, mouse and consumer control (eg: multimedia keys) and can
be paired with up to 4 different devices.


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
    #arduino-cli lib install "NimBLE-Arduino"
    ```

    Also, there needs to be a small patch made to NimBLE, 
    see [ble.cpp](./firmware/ble.cpp).

4. Build

    ```bash
    cd firmware
    ./build
    ```

5. Flash (replace com7 with your serial port)

    ```bash
    ./flash com7
    ```



## Configuring Device

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


## Pairing BLE Devices

Up to 4 paired BLE devices are supported, but only one can be active at a time.  To pair a device:

1. Connect using a serial monitor program
2. Type `pair N` and the device will start advertising itself for pairing (eg: type `pair 0` for the first device)
3. Go to the device to be paired and choose to pair it.
4. You should see connected and authenicated messages in the serial terminal and then a message 
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

### IR Transmit

To transmit an IR signal, send a UDP packet to port 4210 in the following format:

* `uint16_t cmd` - must be 1
* `uint16_t deviceIndex` - a user defined device index from 0 - 15 (see below)
* `uint32_t carrierFreq` - carrier frequency
* `uint32_t gap` - a trailing gap (see below)
* `uint16_t timingData[]` - IR code timing values

(all values are little endian)

The `cmd` value must be 1 and indicates this is an IR transmission packet

The `deviceIndex` is used to enforce gaps between consecutive packets targeted at the same device.  Can be just set to zero
to use the same gap timing between all.

The `carrierFrequency` must be `38000` otherwise the packet is ignored (might add support for others later)

The `gap` value is a minimum time (in microseconds) that the 
device will enforce between the end of one transmission and the start of the next.

The `timingData` is an array of microsecond timing values where event indices are pulses and odd indices are spaces.  If an odd number of values is passed an implicit zero gap is appended.

Note: pronto IR code definitions usually include the `gap` as the last space in the timing data. I recommended to pass that last space timing value as the `gap` parameter and set the trailing timing space to 0.



### Connect BLE Device

Before sending HID packets to a BLE device, the device first needs to be connected:

* `uint16_t cmd` - must be 2
* `uint8_t bleslot` - the BLE virtual device slot to connect

Currently there's no network method to check if the connection succeeded.  Use the serial monitor to check status manually.



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
otherwise the packet will be dropped (check serial monitor if packets are not being delivered).



## NodeJS Library

See [irtx-node](https://github.com/toptensoftware/irtx-node) for library.



## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.