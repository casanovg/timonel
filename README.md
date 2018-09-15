Timonel - ATtiny85 I2C Bootloader
=================================

Timonel is an I2C bootloader for ATtiny85 microcontrollers. The aim is to enable AVR firmware updates in scenarios where there is a more powerful MCU (ESP8266, Arduino, RPi, BeagleBone, etc.) acting as I2C master and one or more Tiny85 as I2C slave performing peripheral functions.

Some possible cases:

* A small robot controlled by Raspberry Pi where the specific functions of each limb are delegated to several Tiny85 through an I2C bus.
* Multi-sensor systems, where each Tiny85 is a node that handles one or more sensors.
* etc ...

In these examples, it's handy to have the chance of updating the AVRs firmware straight from the I2C master. But, so far, I haven't found an I2C bootloader that fits directly the Tiny85, addressing its limitations:
* It doesn't have a specific peripheral for I2C, only the USI.
* It lacks a protected memory area for the bootloader.
* It is not possible to redirect the interruption vectors to the bootloader.
That's why I started writing this one.

Usage:
------
* Install "timonel-I2Cmaster" in an arduino-compatible MCU (It has been tested with ESP8266 ESP01 and NodeMCU).
* Install "timonel-bootloader" on the Tiny85 (bare chips or Digisparks).
* Connect both chips by I2C.
* Compile your program and convert the generated .hex into an array of bytes to be included in "timonel-I2Cmaster". (e.g. const uint8_t payload[size] = { 0xaa, 0xbb, ...}; ). Use "tml-hexparser" for helping to create the array.
* Open an asynchronous terminal (e.g. MobaXterm) connected to the serial port of the I2C master (9600 N 8 1).
* Run the commands shown on screen (more details on the steps necessary for erasing and flashing new firmware will be published later).
* It is also possible to update the bootloader itself thanks to the micronucleus' upgrade program.
