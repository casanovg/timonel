# Timonel Bootloader v1.3 #

This version of the bootloader has several improvements:

* The TWI (I2C) driver implementation with inline functions inside the C source of the bootloader allows a significant memory saving. Now the smaller version "tml-t85-small" occupies ~ 1K byte, leaving 7K bytes available for user applications.

* Different internal clock configurations support has been improved. Now the bootloader adapts its clock speed by modifying the OSCCAL register to compensate fuse configurations in 1, 2 and 8 MHz (At 16 Mhz it works without any type of compensation). This can be done in two ways:
1-The compensation is set at the time of bootloader compilation by reading the variable LOW_FUSE.
2-It adapts at runtime by directly reading the value of the low fuse. This last mode, "AUTO_CLK_TWEAK", allows the bootloader to continue working despite changing the configuration of the fuses, but it takes a little more memory. In either case, the bootloader restores the original configuration after its execution so that the application can function at the speed for which it was designed.

* Some details in the code have been modified to allow the bootloader to work with packet sizes of 32 bytes (1/2 page in an ATtiny85). This gives a marked increase in performance in the user application upload and flash memory reading.

## Compilation ##
The "make-timonel.sh" script has been added to launch make with different configurations. This script even has an argument (--all) that compiles all the preconfigured options, existing in the "configs" directory. The binary files are saved in the "releases" directory. The compilation of this version has been done with the toolchain avr-gcc version 8.3.0 of 64 bits downloaded from the site "http://blog.zakkemble.net/avr-gcc-builds": It is included in the tools directory.

## Flashing ##
The script "flash-timonel-bootloader.sh" allows flashing Timone on the device by using the "avrdude" utility. The supported arguments are2: the name of the binary .hex file (located in the releases directory) and the desired clock speed. The bootloader can be flashed to bare ATtiny85/45/25 chips, Digispark boards and other Tiny85-compatible devices with an AVR programmer (e.g. USBasp).
