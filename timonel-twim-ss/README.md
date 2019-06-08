Timonel TWI master - Single slave
=================================

This test application shows the usage of TWI master libraries.

It is a serial console based application, Arduino-compatible, that allows sending commands to a device that runs the Timonel bootloader. Its main functions are:

* Searches a device running Timonel bootloader on the TWI bus and initialize it.

* Uploads an application to the device. The application to send to the AVR bootloader (payload) is compiled as part of this TWI master application. The utility "timonel-hexparser" is used to convert an AVR application into a TWI master payload.

* Deletes the application from the AVR device memory.

* Optionally, it makes an on-screen dump of all the device's memory for debugging.

The application has been tested on ESP-01 and NodeMCU modules. It is compiled and flashed to the device using __PlatformIO__ over __VS Code__.
