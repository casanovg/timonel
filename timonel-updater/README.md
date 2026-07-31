# Timonel Updater

__NOTE:__ This bootloader updater comes from the [micronucleus](https://github.com/micronucleus) project.

Usage:
------
To make a bootloader update payload without breaking a sweat, use the __make-updater.sh__ script. Run without arguments and it generates a payload based on the default config. The arguments are positional, in this order:

* __CONFIG__: Timonel config to use. (Def=tml-t85-std).

* __FW_NAME__: Name of the .hex file to produce. (Def=timonel).

* __TWI_ADDR__: TWI (I2C) address to assign to the device. Range: 8-35 (Def=11).

* __START_ADDR__: Bootloader start address in device memory. Range: 0-1C00.

* __CLK_SPEED__: Device speed in MHz. Values: 1, 2, 8 or 16 (Def=1).

* __AUTO_TWEAK__: Whether to adjust the device speed at runtime. Valid options: false-true (Def=false).

Examples:
---------

```$ ./make-updater.sh```

Generates a payload from "tml-t85-std" defaults -> FW_NAME=timonel, TWI_ADDR=11, START_ADDR=0x1B80, CLK_SPEED=1 (MHz), AUTO_TWEAK=false.

```$ ./make-updater.sh tml-t85-full```

Generates a payload from the "tml-t85-full" config.

```$ ./make-updater.sh tml-t85-small new-test 17 1B00 8 false```

Generates a payload from the "tml-t85-small" config, assigning TWI address 17 to the device, setting 0x1B00 as the bootloader start, setting the low fuse to 8 MHz and disabling automatic clock tweaking.

Technical Details:
------------------
A summary of how 'upgrade' works.

__Build process:__

1) Run the generate-data.rb ruby script with the new bootloader's hex file:

   ```$ ruby generate-data.rb new_firmware.hex```

   If it gives you trouble, make sure you're on ruby 1.9 — 1.8 is too old!

   generate-data.rb creates bootloader_data.c, which defines variables holding the entire raw bootloader as an array in flash memory. It also figures out and writes in the bootloader's start address. The hex file can be any bootloader that works like USBaspLoader-tiny85 — that's most (all?) tiny85 bootloaders, and probably some for other AVR chips without hardware bootloader support.

2) Build the hex file with make:

   ```make clean; make```

   The upgrader hex is built the usual way, then combined with updater-prefix.hex (written by hand) to prefix a fake interrupt vector table at the start of the upgrader. This is needed because bootloaders like micronucleus and Fast Tiny & Mega Bootloader only work with firmware that starts with an interrupt vector table, since they mangle the table to forward some interrupts to themselves.

3) Upload the resulting upgrade.hex to a chip you can still recover. If it all works out, go ahead and upload it to chips that are harder to recover but otherwise identical.

How it works:
-------------
Taking inspiration from computer viruses, upgrade does this:

1) Brick the chip:
   First it erases the ISR vector table. That leaves the first page full of 0xFF bytes — a NOP sled. If the chip loses power or resets, it won't enter the bootloader; it slides into the upgrader and restarts the process.

2) Erase and write bootloader:
   The new bootloader's flash pages are erased and rewritten from start to finish.

3) Install the trampoline:
   The fake ISR table erased in step one is now written — a trampoline that forwards any requests to the new bootloader's interrupt vector table. At this point the viral upgrader has finished its life cycle and disabled itself. It should never run again, booting straight into the bootloader instead.
