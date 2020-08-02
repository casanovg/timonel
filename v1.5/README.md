![timonel-bootloader](https://github.com/casanovg/timonel/blob/media/timonel-code.png)
## ATtiny85 I2C Bootloader

Timonel is an I2C bootloader for ATtiny85/45/25 microcontrollers. It is designed to enable AVR firmware updates in scenarios where there is a more powerful MCU (ESP8266, ESP32, Arduino, RPi, BeagleBone, etc.) serving as I2C master and one or more ATtiny85 as I2C slaves that perform peripheral functions.

Some cases:

* A small robot controlled by an ESP8266 where each limb specific functions are delegated to several Tiny85 through an I2C bus.
* Multisensor IoT setups, where each Tiny85 is a node that handles one or more sensors.
* etc ...

In these situations, it is quite convenient to be able to update the AVRs' firmware directly from a single entry point, the I2C master. Depending on the main microcontroller type, the ATtiny updates can even be done wirelessly. However, until now (mid-2018), there doesn't seem to be an I2C bootloader that directly suits the TinyX5 family, addressing its various limitations:

* It does not have dedicated hardware to handle I2C, only the USI (Universal Serial Interface).
* Lacks a bootloader protected memory area.
* Unable to redirect interrupt vectors to the bootloader.

That is why this project began ...

## Usage:

* [Install](/timonel-bootloader/README.md#Installation) "timonel.hex" on a Tiny85 (bare chips or Digisparks).
* Build your application program as usual, then use "[tml-hexparser](/timonel-hexparser)" to convert the ".hex" file into a byte array "payload.h" to be included in the "data/payloads" folder of "[timonel-mss-esp8266](https://github.com/casanovg/timonel-mss-esp8266)" or "[timonel-mms-esp8266](https://github.com/casanovg/timonel-mms-esp8266)".
* Use [VS Code](http://code.visualstudio.com) + [PlatformIO](http://platformio.org) to compile and install "[timonel-mss-esp8266](https://github.com/casanovg/timonel-mss-esp8266)" or "[timonel-mms-esp8266](https://github.com/casanovg/timonel-mms-esp8266)" (containing the payload) in an arduino-compatible MCU. It has been tested with ESP8266 ESP01 and NodeMCU. **Note:** The supplied ".bin" file contains a small payload demo that blinks PB1 on the Tiny85.
* Connect both chips by **I2C** (SDA, SCL and ground).
* Open an asynchronous terminal (e.g. [MobaXterm](http://mobaxterm.mobatek.net)) connected to the serial port of the I2C master (115200 N 8 1).
* Run the "timonel-twim-ss" commands shown on screen for erasing and flashing new firmware on the Tiny85.
* It is also possible to update the bootloader itself by using "[timonel-updater](/timonel-updater)" (based on the micronucleus upgrade program).

## Repository organization:
~~~
timonel                           
│
├── timonel-bootloader : Bootloader main folder. It gets built with "avr-gcc" and "make", using the provided scripts.
│   ├── configs        : Several setups to balance features with memory usage. To be called from the "make-timonel.sh" script.
│   ├── releases       : Binary files folder, this is where the compiler output is saved.
│   ├── ...
│   ├─ make-timonel.sh : Bootloader build script. Use "./make-timonel.sh --help" for usage options and parameters.
│   └─ flash-timonel-bootloader.sh : Flashing script. It takes a given binary from "releases" and flashes it with "avrdude".
│
├── timonel-bootloader-io : Bootloader implemented as a PlatformIO experimental project.
│   ├── configs           : Several setups to balance features with memory usage. Selected from "platformio.ini".
│   ├── ...
│   └─ platformio.ini     : This file controls all the settings and building parameters.
│
├── timonel-hexparser   : Utility to convert a ".hex" binary file into a ".h" payload to be included in I2C master apps.
│   ├── appl-flashable  : Put here application firmware ".hex" files.
│   ├── appl-payload    : Here are saved the apps, converted to ".h" files by the hexparser.
│   ├── ...
│   └─ make-payload.sh  : Hexparser firmware conversion script.
│
├── timonel-updater       : Utility to convert a Timonel binary into a bootloader ".h" update payload for am I2C master.
│   ├── tmlupd-flashable  : Put here Timonel bootloader ".hex" binary files.
│   ├── tmlupd-flashable  : Here are saved the ".h" Timonel payloads for updating the bootloader.
│   ├── ...
│   └─ make-updater.sh    : Timonel bootloader updater conversion script.
~~~

## Dependence on other repositories:

#### Libraries

* **[Nb_Micro](https://github.com/casanovg/Nb_Micro)**: Arduino library to control devices that implement the NB command set over an I2C bus.
* **[Nb_TimonelTwiM](https://github.com/casanovg/Nb_TimonelTwiM)**: Arduino library for uploading firmware to a microcontroller running the Timonel bootloader. It uses the NbMicro library to access the I2C bus.
* **[Nb_TwiBus](https://github.com/casanovg/Nb_TwiBus)**: Arduino library to scan the I2C bus in search of connected devices addresses and data. It uses the TimonelTwiM library bootloader object definition.
* **[nb-twi-cmd](https://github.com/casanovg/nb-twi-cmd)**: NB TWI (I2C) command set.

#### Demo I2C master test applications

* **[timonel-mss-esp8266](https://github.com/casanovg/timonel-mss-esp8266)**: Timonel I2C master **single slave**. Serial console-based application that allows sending commands to a device that runs the bootloader from an ESP8266.
* **[timonel-mms-esp8266](https://github.com/casanovg/timonel-mms-esp8266)**: Timonel I2C master **multi slave**. Serial console-based application that runs a loop that flashes, deletes and runs a user application on several Tiny85's running the bootloader from an ESP8266.
* **[timonel-ota-demo](https://github.com/casanovg/timonel-ota-demo)**: This demo application shows a series of steps performed by an ESP8266 I2C master to check a website for updates, retrieve a new firmware file from the internet, and update an ATtiny85 slave microcontroller over the I2C bus.

## Contributing:

Contributions are welcome! If you want to add a new feature, please feel free to create a pull request or open an issue :o)

## Version History:

**v1.5** \- 2020\-07\-03: Functional Release: Optional commands READEEPR and WRITEEPR have been added to read and write data to the EEPROM as well as the READDEVS command that allows reading the device signature, fuses, and lock bits. A few code fixes and a "pre-main" startup file reduction allows getting an additional flash memory page for applications. The overall project repository was restructured, now the I2C libraries and examples are held on separate git repositories to handle the versioning independently. Added an experimental [PlatformIO project](/timonel-bootloader-io) folder to handle the bootloader building in a more structured way. However, for the moment, the [Make version](/timonel-bootloader) is still the recommended one.

**v1.4** \- 2019\-10\-29: Functional Release: Significant memory saving by inlining the TWI driver functions\, now the smaller version "tml\-t85\-small" occupies less than 1 kB\, leaving 7 kB available for user applications\. Speed improvement through a code tuning to transmit 32\-byte packets \(half a page of memory in a Tiny85\)\. User\-application "**autorun**" is now optional. Internal clock configuration support improved. [Interactive master](/timonel-twim-ss) test program improved with streamlined libs (see it [working](http://youtu.be/-7GOMToGvzI)). [Multi-slave master](/timonel-twim-ms) test program added (see it [working](http://youtu.be/PM9X1thrdOY)).

**v1.3** \- 2019\-06\-06: Functional Release: Bootloader inline functions \(smaller code\) and low fuse auto clock tweaking\. Support for 1\, 2\, 8 and 16 MHz clock speed in user\-application mode\. TWI master UploadApplication refactoring\, now supports both types of page address calculation and both modes of **APP\_USE\_TPL\_PG**. Several bug fixes.

**v1.2** \- 2019\-05\-15: Functional Release: "Good\-neighbor" behavior fixes data dump operation interferences among Timonel devices in multi\-device bus setups\. The TWI master functionality has been packed in a couple of Arduino libraries to ease the handling of the several configuration options\. Timonel\-master\-ss firmware shows its usage\. The TWI master code was moved to PlatformIO\.

**v1.1** \- 2018\-10\-29: Functional Release: Optional ReadFlash command added\. Minor tweaks for running Timonel @ 8 MHz\.

**v1.0** \- 2018\-10\-05: Functional Release: Optional features implemented\.

**v0.9** \- 2018\-09\-29: Functional pre\-release: Temporary page buffer and other minor issues solved\.

**v0.8** \- 2018\-09\-16: First functional pre\-release\.

**v0.7** \- 2018\-09\-07: Non\-functional\.

## Credits:

I would like to thank the guys @ [AVRFreaks.net](http://www.avrfreaks.net), specially joeymorin and clawson) for sharing their vast knowledge and technical advice in general. Many thanks also to Donald Papp @ Hackaday for [posting about this](https://hackaday.com/2018/10/20/i2c-bootloader-for-attiny85-lets-other-micros-push-firmware-updates).
