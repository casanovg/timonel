# Timonel Hexparser

This utility converts a ".hex" binary file into a ".h" file which contains a byte array to be included in I2C master applications (e.g. "[timonel-mss-esp8266](https://github.com/casanovg/timonel-mss-esp8266)" or "[timonel-mms-esp8266](https://github.com/casanovg/timonel-mms-esp8266)").

The AVR binary files should be placed into the "appl-flashable" folder. They can be generated using any editor + the avr-gcc toolchain, Atmel Studio 7 or the Arduino IDE. If you use Arduino IDE, the compiled .hex files are a bit hard to find, use these instructions to find them: "https://arduino.stackexchange.com/questions/48431/how-to-get-the-firmware-hex-file-from-a-ino-file-containing-the-code".

Once that you have the desired .hex in the "appl-flashable" folder, just run

```$ ./make-payload.sh appl-flashable/attiny_firmware.hex``` to convert it into a payload.

The script leaves a ".h" file with the same name of the ATtiny firmware file into the "appl-payload" folder.

The resulting payload must then be copied into the I2C master application (e.g. into the "data/payloads" folder of "timonel-mss-esp8266" or "timonel-mms-esp8266"), and the master application must be recompiled and flashed to the master device before being able to flash the payload to the AVR device running Timonel.