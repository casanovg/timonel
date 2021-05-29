# Timonel Bootloader v1.5

Changes from previous version:

* **Slightly smaller bootloader footprint:** some memory got by removing unused pre-checks from the "crt1.S" initialization file (before calling main), and tweaking compiler options. Now smallest bootloader size, when compiled using the "tml-t85-small" @8Mhz configuration, is ~880 bytes (less than 14 pages). This leaves **7.3 Kbytes (114 pages) available for the user application** on an ATtiny85.
* **READDEVS command added:** this optional command allows getting the device signature bytes, lock, and fuse bits from the device.
* **READEEPR and WRITEEPR commands added:** These optional commands are to read and write data to the device's EEPROM. The extended features flag 5 "EEPROM_ACCESS" includes/excludes both commands from the bootloader binary.

## Compilation

The **"make-timonel.sh"** script allows launching "[make](http://www.gnu.org/software/make)" with different configurations. It has several arguments, including **"--all"**, which compiles all the preconfigured options found in the **"configs"** directory. The flashable **".hex"** binary files are saved in the **"releases"** directory.

E.g: <b>`./make-timonel.sh tml-t85-small timonel 13 1B80 1 false;`</b>

* Generates a **"timonel.hex"** binary file based on **"tml-t85-small"** config.
* Assigns the TWI address **13** to the device in bootloader mode.
* Sets **0x1B80** as the bootloader start memory position.
* Sets the device low fuse to operate at **1 MHz** in user-application mode.
* **Disables** automatic clock tweaking.

**Note:** This bootloader version has been compiled with the **"avr-gcc 8.3.0 64-bit"** toolchain downloaded from [this site](http://blog.zakkemble.net/avr-gcc-builds)., it's also included under the "[avr-toolchains](http://github.com/casanovg/avr-toolchains)" repository. The scripts are included mainly to ease to repetitive work of flashing several devices but, of course, the bootloader can be compiled and flashed using avr-gcc and avrdude directly.

## <a id="Installation"></a>Flashing Timonel on the device

The **"flash-timonel-bootloader.sh"** script allows flashing Timonel on the device by using the "[avrdude](http://savannah.nongnu.org/projects/avrdude)" utility with an [AVR programmer](http://www.fischl.de/usbasp). The supported arguments are:

1. Binary file name (without ".hex" extension), located under "releases" directory.
2. Clock speed in MHz: 16, 8, 2, 1.
The bootloader can be flashed to bare ATtiny85/45/25 chips, Digispark boards and other Tiny85-compatible devices using an AVR programmer (e.g. USBasp).

E.g: <b>`./flash-timonel-bootloader.sh timonel 1;`</b>

* Flashes **"timonel.hex"** binary file on the device with avrdude.
* Sets the device's low fuse to run at **1 MHz** (possible values are 1, 2, 8 and 16).

## Setting optional features

The bootloader has several optional features that allow finding the right balance between characteristics, flash memory usage, and performance. They're enabled from the configuration file **"tml-config.mak"** placed inside specific configuration folders (e.g. "tml-t85-cfg", the default one). It's a makefile include that adds or removes sections of the source code that will be part of the ".hex" binary file when compiling it. As a general rule of thumb, more enabled features = bigger bootloader size = less space for user applications. The available options are briefly described below:

* **ENABLE\_LED\_UI**: If this is enabled, the GPIO pin defined by LED\_UI\_PIN is used to display Timonel activity when certain functions are run. This is useful mainly for debugging. PLEASE DISABLE THIS FOR PRODUCTION! IT COULD ACTIVATE SOMETHING CONNECTED TO A POWER SOURCE BY ACCIDENT! (Default: false).
* **AUTO\_PAGE\_ADDR**: Automatic page and trampoline address calculation. If this option is enabled, the bootloader will auto-increase the uploaded data pages addresses when receiving an application from the TWI (I2C) master and it will calculate the trampoline needed to jump to the application on exit. On the other hand, when this is disabled the bootloader becomes smaller but this task is transferred to the TWI master, so this one has to calculate the page addresses and the trampoline. With this option disabled, enabling CMD\_SETPGADDR becomes mandatory, otherwise, the TWI master won't be able to set the pages addresses, the application upload wouldn't be possible. (Default: true).
* **APP\_USE\_TPL\_PG**: This option enables the user application to use the trampoline page when AUTO\_PAGE\_ADDR is enabled. This is more a kind of safety measure than a true memory stretching since enabling this takes 2 extra memory pages. In the end, disabling this allows 1 extra page. If AUTO\_PAGE\_ADDR is disabled, this option is irrelevant since the trampoline page writing duty is transferred to the I2C master. (Default: false).
* **CMD\_SETPGADDR**: When this is enabled, the TWI master can define the starting address of every application memory page being uploaded. If it's disabled, enabling AUTO\_PAGE\_ADDR becomes mandatory. In such cases, applications can only be flashed starting from page 0 since the page address auto-increase works that way. This is OK for most applications. (Default: false).
* **TWO\_STEP\_INIT**: If this is enabled, Timonel expects a two-step initialization from an I2C master before running the exit, memory erase and write commands. This is a safety measure to avoid an unexpected bootloader initialization, which enables memory functions, due to bus noise. When it's disabled, only a single-step initialization is required. (Default: false).
* **USE\_WDT\_RESET**: If this is enabled, the bootloader uses the watchdog timer for resetting instead of jumping to TIMONEL\_START. This reset is more similar to a power-on reset. It could be useful to start the user application from a "cleanest" state if required. (Default: true).
* **APP\_AUTORUN**: If this option is set to false, the uploaded user application will **NOT** start automatically after a timeout when the bootloader is not initialized. In such a case, the TWI master must launch the app execution (Default: true).
* **CMD\_READFLASH**: This option enables the READFLSH command, which is used by the TWI master for dumping the device's whole memory contents for debugging purposes. It can also be useful for backing up the flash memory before flashing a new firmware. (Default: false).
* **AUTO\_CLK\_TWEAK**: When this feature is enabled, the clock speed adjustment is made at run time based on the low fuse setup. It works only for internal CPU clock configurations: RC oscillator or HF PLL. (Default: false).
* **FORCE\_ERASE\_PG**: If this option is enabled, each flash memory page is erased before writing new data. Normally, it shouldn't be necessary to enable it. (Default: false).
* **CLEAR\_BIT\_7\_R31**: This is to avoid that the first bootloader instruction is skipped after restarting without an user application in memory. See: http://www.avrfreaks.net/comment/2561866#comment-2561866. (Default: false).
* **CHECK\_PAGE\_IX**: If this option is enabled, the page index size is checked to ensure that isn't bigger than SPM\_PAGESIZE (64 bytes in an ATtiny85). This keeps the app data integrity in case the master sends wrong page sizes. (Default: false).
* **CMD\_READDEVS**: This option enables the READDEVS command. It allows reading all fuse bits, lock bits, and device signature imprint table. (Default: false).
* **EEPROM_ACCESS**: This option enables the READEEPR and WRITEEPR commands, which allow reading and writing the device EEPROM. (Default: false).
