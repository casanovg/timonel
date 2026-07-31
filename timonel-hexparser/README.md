# Timonel Hexparser

This utility turns a ".hex" binary file into a ".h" file with a byte array, ready to drop into I2C master apps (e.g. "[timonel-mss-esp8266](https://github.com/casanovg/timonel-mss-esp8266)" or "[timonel-mms-esp8266](https://github.com/casanovg/timonel-mms-esp8266)").

Put your AVR binaries in the "appl-flashable" folder. You can build them with any editor + the avr-gcc toolchain, Atmel Studio 7, or the Arduino IDE. With the Arduino IDE the compiled .hex files can be a bit tricky to find — [here's how to locate them](https://arduino.stackexchange.com/questions/48431/how-to-get-the-firmware-hex-file-from-a-ino-file-containing-the-code).

Once your .hex is in "appl-flashable", just run:

```$ ./make-payload.sh appl-flashable/attiny_firmware.hex```

The script drops a ".h" file (same name as your firmware) into the "appl-payload" folder.

Then copy that payload into the I2C master app (e.g. into the "data/payloads" folder of "timonel-mss-esp8266" or "timonel-mms-esp8266"), recompile it, and flash it to the master device. Only then can you flash the payload to the AVR running Timonel.