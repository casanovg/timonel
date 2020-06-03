# Timonel Bootloader v1.4b Experimental #
## Reply Dispatcher "Flag Day"  ##

This bootloader version has several improvements and experimental features:

* Command reply switch-case replaced by a dispatch table setup. It works well, but it takes more memory than the switch-case version.

* Optional TIMEOUT_EXIT feature added to allow selecting whether the user application will run automatically after a timeout when the bootloader is not initialized, or, it will only be run controlled by the TWI master. Optional CHECK_EMPTY_FL feature was deprecated since the READFLSH command can be used for the same purpose.

* The TWI (I2C) driver implementation, with inline functions inside the bootloader source code, allows significant memory saving. Now the smaller version "tml-t85-small" occupies ~ 1K byte, leaving 7K bytes available for user applications.

* Different internal-clock configurations support has been improved. Now the bootloader adapts its clock speed by modifying the OSCCAL register to compensate fuse configurations in 1, 2 and 8 MHz (At 16 Mhz it works without any type of compensation). This can be done in two ways:
1-The compensation is configured at the time of bootloader compilation by setting the LOW_FUSE variable.
2-It adapts at runtime by directly reading the value of the low fuse. This last mode is selected by enabling "AUTO_CLK_TWEAK". It allows the bootloader to continue working without recompiling despite changing the fuses settings, but it takes a little more memory. In both cases, the bootloader restores the original configuration after its execution so that the application can function at the speed for which it was designed.

* The code was adjusted to allow the bootloader to transmit and receive 32-byte packets (1/2 page of an ATtiny85). This produces a significant performance increase to the user application upload and flash memory reading functions.

## Compilation ##
A script has been added to launch make with different configurations: "make-timonel.sh". This script has several arguments, including "--all", which compiles all the preconfigured options found in the "configs" directory. The flashable ".hex" binary files are saved in the "releases" directory. This bootloader version has been compiled with the "avr-gcc 8.3.0 64-bit" toolchain downloaded from this site: "http://blog.zakkemble.net/avr-gcc-builds", it's also included under the tools directory. The scripts are included mainly to ease to repetitive work of flashing several devices but, of course, the bootloader can be compiled and flashed using avr-gcc and avrdude directly.

## Flashing ##
The script "flash-timonel-bootloader.sh" allows flashing Timonel on the device by using the "avrdude" utility. The supported arguments are 2: 1-Binary file name (without ".hex" extension), located under "releases" directory.
2-Clock speed in MHz: 16, 8, 2, 1.
The bootloader can be flashed to bare ATtiny85/45/25 chips, Digispark boards and other Tiny85-compatible devices using an AVR programmer (e.g. USBasp).

## Optional features ##
This bootloader version has several optional features that allow finding the right balance between characteristics, flash memory usage, and performance. They're enabled from the configuration file "tml-config.mak" placed inside specific configuration folders (e.g. "tml-t85-cfg", the default one). It's a makefile include that adds or removes sections of the source code that will be part of the ".hex" binary file when compiling it. As a general rule of thumb, more enabled features = bigger bootloader size = less space for user applications. The available options are briefly described below:

* __ENABLE_LED_UI__: If this is enabled, the GPIO ping defined by LED_UI_PIN is used to display Timonel activity when certain functions are run. This is useful mainly for debugging. PLEASE DISABLE THIS FOR PRODUCTION! IT COULD ACTIVATE SOMETHING CONNECTED TO A POWER SOURCE BY ACCIDENT! (Default: false).
           
* __AUTO_PAGE_ADDR__: Automatic page and trampoline address calculation. If this option is enabled, the bootloader will auto-increase the uploaded data pages addresses when receiving an application from the TWI (I2C) master and it will calculate the trampoline needed to jump to the application on exit. On the other hand, when this is disabled the bootloader becomes smaller but this task is transferred to the TWI master, so this one has to calculate the page addresses and the trampoline. With this option disabled, enabling CMD_SETPGADDR becomes mandatory, otherwise, the TWI master won't be able to set the pages addresses, the application upload wouldn't be possible. (Default: true).
                                
* __APP_USE_TPL_PG__: This option enables the user application to use the trampoline page when AUTO_PAGE_ADDR is enabled. This is more a kind of safety measure than a true memory stretching since enabling this takes 2 extra memory pages. In the end, disabling this allows 1 extra page. If AUTO_PAGE_ADDR is disabled, this option is irrelevant since the trampoline page writing duty is transferred to the I2C master. (Default: false).
                                
* __CMD_SETPGADDR__: When this is enabled, the TWI master can define the starting address of every application memory page being uploaded. If it's disabled, enabling AUTO_PAGE_ADDR becomes mandatory. In such cases, applications can only be flashed starting from page 0 since the page address auto-increase works that way. This is OK for most applications. (Default: false).

* __TWO_STEP_INIT__: If this is enabled, Timonel expects a two-step initialization from an I2C master before running the exit, memory erase and write commands. This is a safety measure to avoid bootloader initialization, which enables memory functions, due to noise. When it's disabled, only a single-step initialization is required. (Default: false).

* __USE_WDT_RESET__: If this is enabled, the bootloader uses the watchdog timer for resetting instead of jumping to TIMONEL_START. This reset is more similar to a power-on reset. It could be useful to start the user application from a "cleanest" state if required. (Default: true).

* __CHECK_EMPTY_FL__: If this is enabled, when running GETTMNLV command, the bootloader will read the first 100 flash memory positions to check if there is an application (or some other data) loaded in flash memory. The true/false outcome is informed as part of the GETTMNLV command reply packet. (Default: false).

* __CMD_READFLASH__: This option enables the READFLSH command, which is used by the TWI master for dumping the device's whole memory contents for debugging purposes. It can also be useful for backing up the flash memory before flashing a new firmware. (Default: false).

* __AUTO_CLK_TWEAK__: When this feature is enabled, the clock speed adjustment is made at run time based on the low fuse setup. It works only  for internal CPU clock configurations: RC oscillator or HF PLL. (Default: false).

* __FORCE_ERASE_PG__: If this option is enabled, each flash memory page is erased before writing new data. Normally, it shouldn't be necessary to enable it. (Default: false).

* __CLEAR_BIT_7_R31__: This is to avoid that the first bootloader instruction is skipped after restarting without an user application in memory. See: http://www.avrfreaks.net/comment/2561866#comment-2561866. (Default: false).
                                    
* __CHECK_PAGE_IX__: If this option is enabled, the page index size is checked to ensure that isn't bigger than SPM_PAGESIZE (64 bytes in an ATtiny85). This keeps the app data integrity in case the master sends wrong page sizes. (Default: false).
