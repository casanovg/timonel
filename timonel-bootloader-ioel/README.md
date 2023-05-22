# Timonel Bootloader v1.6 "Ext-Lib" - PlatformIO experimental project

This folder contains the same bootloader version and functionality as the "[Make version](/timonel-bootloader-el)", but it was implemented as a [PlatformIO](http://platformio.org) experimental project to handle building in a more structured way. Some advantages of using this platform are:

* Development standardization of all components on a single platform: bootloader master and slave sides, I2C libraries, applications, etc. This is possible even with different frameworks: ESP8266, AVR, Arduino, etc.
* One-click compilation, handled by structured ".ini" and "JSON" files.
* Simple and orderly updating of platforms and libraries, with a centralized registry of library versions.
* GitHub integration, Editor IntelliSense (VS Code).

It is possible that in the future this version will supersede the "Make version", but so far (July 2020), several issues have to be solved:

* The handling of the various bootloader settings in ".ini" files works, but is not yet polished enough.
* To resolve the previous point, custom variables have been added in the ".ini" files that are unknown by PlatformiIO, throwing warnings at the time of compilation.
* The size of the binary files obtained in all configurations is substantially larger than their counterparts built with Make.
* The way to correctly control the platform default compiler version has not been found yet, nor the options that Platformio passes to the compiler and the linker.

## Compilation

It is possible to compile all the bootloader configurations with a single click using the **"Build"** option of the PlatformIO project tasks, from the icon located in the editor footer or the "platform run" command.

The bootloader configurations to be built are controlled by lines added under the "extra_configs" section of the "platformio" option in the platformio.ini file. Each line includes a ".ini" file from the "configs" folder with the specific environment configuration parameters.

To build a single configuration (or just a few), it can be added in the "default_envs" section.

## <a id="Installation"></a>Flashing Timonel on the device

To update the bootloader on the device, use the **"PlatformIO Upload"** command found in project tasks, in the editor footer, or through "platformio run" in the command line.

E.g. **"platformio.exe run -e tml-t85-std --target upload"** will build and flash the Timonel standard configuration into the device using the programmer configured in platformio.ini (default: [USBasp](http://www.fischl.de/usbasp)).

## Setting optional features

The bootloader has several optional features that allow finding the right balance between characteristics, flash memory usage, and performance. They're enabled from the configuration file **"tml-config.mak"** placed inside specific configuration folders (e.g. "tml-t85-cfg", the default one). It's a makefile include that adds or removes sections of the source code that will be part of the ".hex" binary file when compiling it. As a general rule of thumb, more enabled features = bigger bootloader size = less space for user applications. The available options are briefly described below:

* **ENABLE\_LED\_UI**: If this is enabled, the GPIO pin defined by LED\_UI\_PIN is used to display Timonel activity when certain functions are run. This is useful mainly for debugging. PLEASE DISABLE THIS FOR PRODUCTION! IT COULD ACTIVATE SOMETHING CONNECTED TO A POWER SOURCE BY ACCIDENT! (Default: false).
* **AUTO\_PAGE\_ADDR**: Automatic page and trampoline address calculation. If this option is enabled, the bootloader will auto-increase the uploaded data pages addresses when receiving an application from the TWI (I2C) master and it will calculate the trampoline needed to jump to the application on exit. On the other hand, when this is disabled the bootloader becomes smaller but this task is transferred to the TWI master, so this one has to calculate the page addresses and the trampoline. With this option disabled, enabling CMD\_SETPGADDR becomes mandatory, otherwise, the TWI master won't be able to set the pages addresses, the application upload wouldn't be possible. (Default: true).
* **APP\_USE\_TPL\_PG**: This option enables the user application to use the trampoline page when AUTO\_PAGE\_ADDR is enabled. This is more a kind of safety measure than a true memory stretching since enabling this takes 2 extra memory pages. In the end, disabling this allows 1 extra page. If AUTO\_PAGE\_ADDR is disabled, this option is irrelevant since the trampoline page writing duty is transferred to the I2C master. (Default: false).
* **CMD\_SETPGADDR**: When this is enabled, the TWI master can define the starting address of every application memory page being uploaded. If it's disabled, enabling AUTO\_PAGE\_ADDR becomes mandatory. In such cases, applications can only be flashed starting from page 0 since the page address auto-increase works that way. This is OK for most applications. (Default: false).
* **TWO\_STEP\_INIT**: If this is enabled, Timonel expects a two-step initialization from an I2C master before running the exit, memory erase and write commands. This is a safety measure to avoid an unexpected bootloader initialization, which enables memory functions, due to bus noise. When it's disabled, only a single-step initialization is required. (Default: false).
* **USE\_WDT\_RESET**: If this is enabled, the bootloader uses the watchdog timer for resetting instead of jumping to TIMONEL\_START. This reset is more similar to a power-on reset. It could be useful to start the user application from a "cleanest" state if required. (Default: true).
* ~~**CHECK\_EMPTY\_FL**: If this is enabled, when running GETTMNLV command, the bootloader will read the first 100 flash memory positions to check if there is an application (or some other data) loaded in flash memory. The true/false outcome is informed as part of the GETTMNLV command reply packet. (Default: false)~~. **Deprecated  in v1.4!**
* **APP\_AUTORUN**: If this option is set to false, the uploaded user application will **NOT** start automatically after a timeout when the bootloader is not initialized. In such a case, the TWI master must launch the app execution (Default: true).
* **CMD\_READFLASH**: This option enables the READFLSH command, which is used by the TWI master for dumping the device's whole memory contents for debugging purposes. It can also be useful for backing up the flash memory before flashing a new firmware. (Default: false).
* **AUTO\_CLK\_TWEAK**: When this feature is enabled, the clock speed adjustment is made at run time based on the low fuse setup. It works only for internal CPU clock configurations: RC oscillator or HF PLL. (Default: false).
* **FORCE\_ERASE\_PG**: If this option is enabled, each flash memory page is erased before writing new data. Normally, it shouldn't be necessary to enable it. (Default: false).
* **CLEAR\_BIT\_7\_R31**: This is to avoid that the first bootloader instruction is skipped after restarting without an user application in memory. See: http://www.avrfreaks.net/comment/2561866#comment-2561866. (Default: false).
* **CHECK\_PAGE\_IX**: If this option is enabled, the page index size is checked to ensure that isn't bigger than SPM\_PAGESIZE (64 bytes in an ATtiny85). This keeps the app data integrity in case the master sends wrong page sizes. (Default: false).
* **CMD\_READDEVS**: This option enables the READDEVS command. It allows reading all fuse bits, lock bits, and device signature imprint table. (Default: false).
* **EEPROM_ACCESS**: This option enables the READEEPR and WRITEEPR commands, which allow reading and writing the device EEPROM. (Default: false).
