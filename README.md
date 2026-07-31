![timonel-bootloader](https://github.com/casanovg/timonel/blob/media/timonel-code.png)
## ATtiny85 I2C Bootloader

Timonel is an I2C bootloader for ATtiny85/45/25 microcontrollers. It lets you update AVR firmware when there's a more powerful MCU around (ESP8266, ESP32, Arduino, RPi, BeagleBone, etc.) acting as the I2C master, with one or more ATtiny85s as I2C slaves doing peripheral tasks.

Some examples:

* A small robot controlled by an ESP8266, where each limb's tasks are handled by a Tiny85 over an I2C bus.
* Multisensor IoT setups, where each Tiny85 is a node taking care of one or more sensors.
* etc. — you get the idea.

In these setups it's really handy to update the AVRs' firmware from a single entry point: the I2C master. Depending on the main MCU, updates can even go over the air. But back in mid-2018 there wasn't an I2C bootloader that really fit the TinyX5 family and worked around its limitations:

* No dedicated hardware for I2C — just the USI (Universal Serial Interface).
* No protected memory area for the bootloader.
* No way to redirect interrupt vectors to the bootloader.

So this project started...

## Usage:

* [Install](/timonel-bootloader/README.md#Installation) "timonel.hex" on a Tiny85 (bare chips or Digisparks).
* Build your application as usual, then use "[tml-hexparser](/timonel-hexparser)" to turn the ".hex" file into a byte array "payload.h", and drop it into the "data/payloads" folder of "[timonel-mss-esp8266](https://github.com/casanovg/timonel-mss-esp8266)" or "[timonel-mms-esp8266](https://github.com/casanovg/timonel-mms-esp8266)".
* Use [VS Code](http://code.visualstudio.com) + [PlatformIO](http://platformio.org) to compile and flash "[timonel-mss-esp8266](https://github.com/casanovg/timonel-mss-esp8266)" or "[timonel-mms-esp8266](https://github.com/casanovg/timonel-mms-esp8266)" (which includes the payload) on an Arduino-compatible MCU. It's been tested with the ESP8266 ESP01 and NodeMCU. **Note:** the included ".bin" file has a small demo payload that blinks PB1 on the Tiny85.
* Hook up both chips over **I2C** (SDA, SCL and ground).
* Open a terminal (e.g. [MobaXterm](http://mobaxterm.mobatek.net)) on the I2C master's serial port (115200 N 8 1).
* Run the "timonel-twim-ss" commands shown on screen to erase and flash new firmware on the Tiny85.
* You can also update the bootloader itself with "[timonel-updater](/timonel-updater)" (based on the micronucleus upgrade program).

## Repository organization:
~~~
timonel                           
│
├── timonel-bootloader : Bootloader main folder. Built with "avr-gcc" and "make", using the provided scripts.
│   ├── configs        : Several setups to balance features with memory usage. Called from the "make-timonel.sh" script.
│   ├── releases       : Where the compiled binaries end up.
│   ├── ...
│   ├─ make-timonel.sh : Bootloader build script. Run "./make-timonel.sh --help" for options and parameters.
│   └─ flash-timonel-bootloader.sh : Flashing script. Takes a binary from "releases" and flashes it with "avrdude".
│
├── timonel-bootloader-io : Bootloader as a PlatformIO experimental project.
│   ├── configs           : Several setups to balance features with memory usage. Selected from "platformio.ini".
│   ├── ...
│   └─ platformio.ini     : Controls all the settings and build parameters.
│
├── timonel-bootloader-el : Same bootloader as "timonel-bootloader" (Make version), but with the USI-based, interrupt-free I2C driver as an external library.
│   ├── configs           : Several setups to balance features with memory usage. Called from the "make-timonel.sh" script.
│   ├── releases          : Where the compiled binaries end up.
│   ├── ...
│   ├─ make-timonel.sh    : Bootloader build script. Run "./make-timonel.sh --help" for options and parameters.
│   └─ flash-timonel-bootloader.sh : Flashing script. Takes a binary from "releases" and flashes it with "avrdude".
│
├── timonel-bootloader-ioel : Same bootloader as "timonel-bootloader-el", but as a PlatformIO experimental project.
│   ├── configs           : Several setups to balance features with memory usage. Selected from "platformio.ini".
│   ├── ...
│   └─ platformio.ini     : Controls all the settings and build parameters.
│
├── timonel-hexparser   : Turns a ".hex" file into a ".h" payload for I2C master apps.
│   ├── appl-flashable  : Put your application ".hex" files here.
│   ├── appl-payload    : The converted ".h" payloads land here.
│   ├── ...
│   └─ make-payload.sh  : Hexparser conversion script.
│
├── timonel-updater       : Turns a Timonel binary into a bootloader ".h" update payload for an I2C master.
│   ├── tmlupd-flashable  : Put Timonel bootloader ".hex" files here.
│   ├── tmlupd-payload    : The ".h" bootloader update payloads land here.
│   ├── ...
│   └─ make-updater.sh    : Bootloader updater conversion script.
~~~

## Dependencies on other repositories:

#### Libraries

* **[Nb_Micro](https://github.com/casanovg/Nb_Micro)**: Arduino library to control devices that speak the NB command set over an I2C bus.
* **[Nb_TimonelTwiM](https://github.com/casanovg/Nb_TimonelTwiM)**: Arduino library for uploading firmware to a chip running the Timonel bootloader. It uses NbMicro to talk to the I2C bus.
* **[Nb_TwiBus](https://github.com/casanovg/Nb_TwiBus)**: Arduino library to scan the I2C bus for connected device addresses and data. It uses the TimonelTwiM bootloader object definition.
* **[nb-twi-cmd](https://github.com/casanovg/nb-twi-cmd)**: The NB TWI (I2C) command set.

#### Demo I2C master apps

* **[timonel-mss-esp8266](https://github.com/casanovg/timonel-mss-esp8266)**: Timonel I2C master for a **single slave**. Serial-console app to send commands to a device running the bootloader, from an ESP8266.
* **[timonel-mms-esp8266](https://github.com/casanovg/timonel-mms-esp8266)**: Timonel I2C master for **multiple slaves**. Serial-console app that loops around flashing, erasing and running a user app on several Tiny85s running the bootloader, from an ESP8266.
* **[timonel-ota-demo](https://github.com/casanovg/timonel-ota-demo)**: Shows an ESP8266 I2C master checking a website for updates, **grabbing a new firmware file from the internet**, and updating an ATtiny85 slave over the I2C bus.

## Contributing:

Contributions are welcome! Want to add a feature? Feel free to open a pull request, file an issue, or start a [discussion](https://github.com/casanovg/timonel/discussions). :o)

## Version History:

**v1.6** \- 2023\-05\-22: All multi\-byte fields in the protocol now use the same **little\-endian** (LSB first) byte order, as agreed in [discussion #28](https://github.com/casanovg/timonel/discussions/28). Data packets bumped up to **64 bytes** (a full SPM page on an ATtiny85). Support extended to more USI\-based AVR families (ATtinyX4/X5, ATtiny2313, ATtinyX7, ATtiny26, ATtiny43U). Use "|" instead of "+" when building flash addresses, saving 4 bytes each time. Fixed "inline" keyword inconsistencies and improved the scripts (STK500/COM port option, "#!/bin/bash" shebang, path backslashes). Added the "tml\-t85\-test\-comm" configuration for easy startup.

**v1.5** \- 2020\-07\-03: Added READEEPR and WRITEEPR to read and write EEPROM data, plus the READDEVS command to read the device signature, fuses and lock bits. A few code fixes and a smaller "pre-main" startup file free up an extra flash page for apps. The repo was reorganized: I2C libraries and examples now live in their own repos so they can be versioned independently. Added an experimental [PlatformIO project](/timonel-bootloader-io) for more structured builds, though the [Make version](/timonel-bootloader) is still the recommended one.

**v1.4** \- 2019\-10\-29: Big memory savings by inlining the TWI driver functions — "tml\-t85\-small" now takes under 1 kB, leaving 7 kB for user apps. Speed boost by sending 32\-byte packets (half a page of memory on a Tiny85). "**autorun**" for user apps is now optional, and internal clock config support got better. [Interactive master](https://github.com/casanovg/timonel-mss-esp8266) improved with slimmer libs (see it [working](http://youtu.be/-7GOMToGvzI)), plus a new [multi\-slave master](https://github.com/casanovg/timonel-mms-esp8266) (see it [working](http://youtu.be/PM9X1thrdOY)).

**v1.3** \- 2019\-06\-06: Inline bootloader functions (smaller code) and low\-fuse auto clock tweaking. Support for 1, 2, 8 and 16 MHz clocks in user\-app mode. Refactored the TWI master's UploadApplication, which now supports both page address calculation types and both **APP\_USE\_TPL\_PG** modes. Several bug fixes.

**v1.2** \- 2019\-05\-15: "Good\-neighbor" behavior stops data dumps from interfering between Timonel devices on a multi\-device bus. The TWI master code was packed into a couple of Arduino libraries to make the many config options easier to handle, and moved to PlatformIO.

**v1.1** \- 2018\-10\-29: Added the optional ReadFlash command. Minor tweaks to run Timonel at 8 MHz.

**v1.0** \- 2018\-10\-05: Optional features implemented.

**v0.9** \- 2018\-09\-29: Functional pre\-release: temporary page buffer and other minor issues fixed.

**v0.8** \- 2018\-09\-16: First functional pre\-release.

**v0.7** \- 2018\-09\-07: Non\-functional.

## Credits:

Thanks to the folks at [AVRFreaks.net](http://www.avrfreaks.net), especially joeymorin and clawson, for all the knowledge and technical advice. And many thanks to Donald Papp at Hackaday for [posting about this](https://hackaday.com/2018/10/20/i2c-bootloader-for-attiny85-lets-other-micros-push-firmware-updates).
