Timonel - ATtiny85 I2C Bootloader
=================================

Timonel is an I2C bootloader for ATtiny85/45/25 microcontrollers. The aim is to enable AVR firmware updates in scenarios where there is a more powerful MCU (ESP8266, Arduino, RPi, BeagleBone, etc.) acting as I2C master and one or more Tiny85 as I2C slave performing peripheral functions.

Some possible cases:

* A small robot controlled by Raspberry Pi where the specific functions of each limb are delegated to several Tiny85 through an I2C bus.
* Multi-sensor systems, where each Tiny85 is a node that handles one or more sensors.
* etc ...

In these situations, it's handy to be able to update the AVRs firmware straight from the I2C master. But, so far (mid-2018), I haven't found an I2C bootloader that fits directly the TinyX5 family, addressing its several limitations:
* It doesn't have dedicated hardware to handle I2C, only the USI (Universal Serial Interface).
* It lacks a protected memory area for the bootloader.
* It is not possible to redirect the interruption vectors to the bootloader.

That's why I started writing this one.

Usage:
------
* Install "timonel.hex" on the Tiny85 (bare chips or Digisparks).
* Compile your program and convert the generated .hex into an array of bytes to be included in "timonel-I2Cmaster". (e.g. const uint8_t payload[size] = { 0xaa, 0xbb, ...}; ). Use "tml-hexparser" for helping to create the array.
* Install "timonel-twim-ss.bin" containing the generated payload in an arduino-compatible MCU (It has been tested with ESP8266 ESP01 and NodeMCU).
* Connect both chips by I2C.
* Open an asynchronous terminal (e.g. MobaXterm) connected to the serial port of the I2C master (9600 N 8 1).
* Run the commands shown on screen for erasing and flashing new firmware on the Tiny85 (more details on the steps necessary will be published later).
* It is also possible to update the bootloader itself thanks to the micronucleus' upgrade program.

Contributing:
-------------
Contributions are welcome! If you want to add a new feature, please feel free to create a pull request or open an issue :o)

Version History:
----------------
v1.2 - 2019-05-15: "Good-neighbor" behavior: Fixes data dump operation interferences among Timonel devices in multi-device bus setups. The TWI master functionality has been packed in a couple of Arduino libraries to ease the handling of the several configuration options. Timonel-master-ss firmware shows its usage. The TWI master code was moved to PlatformIO.

v1.1 - 2018-10-29: Functional Release: Optional ReadFlash command added. Minor tweaks for running Timonel @ 8 MHz.

v1.0 - 2018-10-05: Functional Release: Optional features implemented. See it [working](https://youtu.be/-7GOMToGvzI).

v0.9 - 2018-09-29: Functional pre-release: Temporary page buffer and other minor issues solved.

v0.8 - 2018-09-16: First functional pre-release.

v0.7 - 2018-09-07: Non-functional.

v0.4 - 2018-08-10: Non-functional.

Credits:
--------
I would like to thank the guys @ [AVRFreaks.net](http://www.avrfreaks.net), specially joeymorin and clawson) for sharing their vast knowledge and technical advice in general. Many thanks also to Donald Papp @ [Hackaday](http://hackaday.com) for [posting about this](https://hackaday.com/2018/10/20/i2c-bootloader-for-attiny85-lets-other-micros-push-firmware-updates).
