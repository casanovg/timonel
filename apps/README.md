# Test Apps v1.4 #

This folder contains simple apps that can be used to test the bootloader functionality.

Choose any of then and compile it. The resulting ".hex" file has to be converted into a byte-array by using "timonel-hexparser". After that, the "payload.h" file obtained has to be included in the "/data/payloads" folder of "timonel-twim-ss" or "timonel-twim-ss" to be able to flash it into the T85.

* avr-blink-twis: Simple led blink demo with I2C controllable through a serial console.

* avr-native-blink: Simple AVR blink.

* bare-t85-blink-io: Simple blink compiled with platformio.
