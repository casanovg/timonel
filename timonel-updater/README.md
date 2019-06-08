Timonel Updater
===============

__NOTE:__ This bootloader updater has been included from the micronucleus project: github.com/micronucleus

Usage:
------
To simplify the step necesary to generate a bootloader update payload, use the __make-updater.sh__ script. If the script is run without arguments, it will generate a default-configuration based bootloader payload. The supported arguments are positional, as follows:

__CONFIG__      Timonel configuration option to use. (Def=tml-t85-std).
__FW_NAME__     Name of the .hex binary file to produce. (Def=timonel).
__TWI_ADDR__    TWI (I2C) address to assign to the device. Range: 8-35 (Def=11).
__START_ADDR__  Bootloader start address in the device memory. Range: 0-1C00."
__CLK_SPEED__   Device speed settings (in MHz). Values: 1, 2, 8 or 16 (Def=1).
__AUTO_TWEAK__  Defines if the device speed adjustments will be made at run time. Valid options: false-true (Def=false)."

Examples:
---------

```$ ./make-updater.sh```
  
Generates a payload file based on "tml-t85-std" defaults->FW_NAME=timonel, TWI_ADDR=11, START_ADDR=0x1B80, CLK_SPEED=1 (MHz), AUTO_TWEAK=false.

```$ ./make-updater tml-t85-full```
  
Generates a payload file based on "tml-t85-full" config.

```$ ./make-updater.sh tml-t85-small new-test 17 1B00 8 false```

Generates a payload file based on \"tml-t85-small\" config, assigning TWI address 17 to the device,
setting 0x1B00 device memory position as bootloader start, setting the device low fuse to operate at 8 MHz and disabling automatic clock tweaking.

Technical Details
-----------------
A summary of how 'upgrade' works

__Build process:__

1) Manually run the generate-data.rb ruby script with the new bootloader's hex file:
   
   $ ruby generate-data.rb new_firmware.hex
   
   If you have trouble running it, make sure you're using ruby version 1.9. 1.8 is too old!
   
   generate-data.rb creates the bootloader_data.c file, which defines some variables containing
   the entire raw byte data of the bootloader as an array stored in flash memory. It also
   calculates and writes in the start address of the bootloader. The hex file supplied can be any
   bootloader which works similarly to USBaspLoader-tiny85 - which is most (all?) tiny85 bootloaders
   and likely some for other avr chips which lack hardware bootloader stuff.
   
2) Generate the hex file using make:
   
   make clean; make
   
   The upgrader hex file is built in the usual way, then combined with upgrade-prefix.hex (which
   I wrote by hand) to prefix a fake interrupt vector table in the start of the upgrader. This is
   necessary because bootloaders like micronucleus and Fast Tiny & Mega Bootloader only work with
   firmwares which begin with an interrupt vector table, because of the way they mangle the table
   to forward some interrupts to themselves.
   
3) Upload the resulting upgrade.hex file to a chip you have some means of recovering. If all works
   correctly, consider now uploading it to other chips which maybe more difficult to recover but are
   otherwise identical.

How it works
------------

Taking inspiration from computer viruses, when upgrade runs it goes through this process:

1) Brick the chip:
   The first thing upgrade does is erase the ISR vector table. Erasing it sets the first page to
   0xFF bytes - creating a NOP sled. If the chip looses power or otherwise resets, it wont enter
   the bootloader, sliding in to the upgrader restarting the process.

2) erase and write bootloader:
   The flash pages for the new bootloader are erased and rewritten from start to finish.
   
3) install the trampoline:
   The fake ISR table which was erased in step one is now written to - a trampoline is added, simply
   forwarding any requests to the new bootloader's interrupt vector table. At this point the viral
   upgrader has completed it's life cycle and has disabled itself. It should never run again, booting
   directly in to the bootloader instead.


