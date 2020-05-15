#!/bin/sh
avrdude -c USBasp -p attiny85 -B3 -U flash:w:avr-blink-twis.hex:i -U lfuse:w:0x62:m -U hfuse:w:0xdf:m -U efuse:w:0xff:m
