# Timonel Bootloader v1.6 "Ext-Lib" - PlatformIO experimental project

This folder holds the same bootloader version and functionality as the "[Make version](/timonel-bootloader-el)", but as an experimental [PlatformIO](http://platformio.org) project to make building a bit more structured. Some upsides:

* Everything on one platform: bootloader master and slave sides, I2C libraries, apps, etc. It even works across different frameworks — ESP8266, AVR, Arduino, and so on.
* One-click builds, driven by structured ".ini" and "JSON" files.
* Simple, tidy updates of platforms and libraries, with a central registry of library versions.
* GitHub integration and editor IntelliSense (VS Code).

This version might eventually replace the "Make version", but for now (April 2023) there are still a few rough edges:

* The bootloader settings in ".ini" files work, but aren't polished yet.
* To work around that, the ".ini" files use custom variables PlatformIO doesn't know about, which throw warnings at compile time.
* The binaries for every config come out noticeably bigger than their Make-built counterparts.
* Still no clean way to pin the exact compiler version PlatformIO uses, or the flags it passes to the compiler and linker.

## Compilation

You can build all bootloader configs with a single click using the **"Build"** option of the PlatformIO project tasks, from the icon in the editor footer or the "platform run" command.

Which configs get built is controlled by the lines under the "extra_configs" section of the "platformio" option in the platformio.ini file. Each line pulls in a ".ini" file from the "configs" folder with that environment's settings.

To build just one config (or a few), add it to the "default_envs" section instead.

## <a id="Installation"></a>Flashing Timonel on the device

To update the bootloader on a device, use the **"PlatformIO Upload"** command from the project tasks, the editor footer, or "platformio run" on the command line.

E.g. **"platformio.exe run -e tml-t85-std --target upload"** builds and flashes the standard Timonel config using the programmer set in platformio.ini (default: [USBasp](http://www.fischl.de/usbasp)).

## Optional features

The bootloader has several optional features so you can find the right balance between features, flash usage, and performance. They're enabled from the **"tml-t85-*.ini"** config files in the "configs" folder (e.g. "tml-t85-std.ini", the default one). Each ".ini" file sets up a build environment and adds or removes chunks of code from the final binary through its "build_flags" settings. As a rule of thumb: more features = bigger bootloader = less room for user apps. Here's what's available:

* **ENABLE\_LED\_UI**: Uses the GPIO pin set in LED\_UI\_PIN to show Timonel activity while certain functions run. Handy for debugging. PLEASE DISABLE THIS FOR PRODUCTION! IT COULD ACTIVATE SOMETHING CONNECTED TO A POWER SOURCE BY ACCIDENT! (Default: false).
* **AUTO\_PAGE\_ADDR**: The bootloader auto-increases the uploaded page addresses and calculates the trampoline needed to jump to the app on exit. If disabled, the bootloader gets smaller but the TWI master has to figure out page addresses and the trampoline instead. With this off, CMD\_SETPGADDR becomes mandatory, otherwise uploads won't work. (Default: true).
* **APP\_USE\_TPL\_PG**: Lets the user app use the trampoline page when AUTO\_PAGE\_ADDR is on. More of a safety measure than real space-saving, since it costs 2 extra pages (turning it off frees 1). Irrelevant if AUTO\_PAGE\_ADDR is off. (Default: false).
* **CMD\_SETPGADDR**: Lets the TWI master set the start address of every page it uploads. If disabled, AUTO\_PAGE\_ADDR must be on, and apps can only be flashed from page 0. Fine for most apps. (Default: false).
* **TWO\_STEP\_INIT**: Timonel expects a two-step init from the I2C master before running exit, erase, and write commands. A safety measure against accidental init (and the memory functions it unlocks) from bus noise. Off means a single-step init is enough. (Default: false).
* **USE\_WDT\_RESET**: Uses the watchdog timer to reset instead of jumping to TIMONEL\_START. Closer to a real power-on reset, which can be handy if you want the app to start from the cleanest state possible. (Default: true).
* **APP\_AUTORUN**: If false, the uploaded app will **NOT** auto-start after the timeout when the bootloader isn't initialized; the TWI master has to launch it. (Default: true).
* **CMD\_READFLASH**: Enables the READFLSH command, which dumps the whole flash contents over TWI for debugging, or to back up the firmware before flashing something new. (Default: false).
* **AUTO\_CLK\_TWEAK**: Adjusts the clock speed at runtime based on the low fuse. Only works with internal clock setups: RC oscillator or HF PLL. (Default: false).
* **FORCE\_ERASE\_PG**: Erases each flash page before writing new data. Usually not needed. (Default: false).
* **CLEAR\_BIT\_7\_R31**: Prevents the first bootloader instruction from being skipped after a restart with no user app in memory. See: http://www.avrfreaks.net/comment/2561866#comment-2561866. (Default: false).
* **CHECK\_PAGE\_IX**: Checks that the page index doesn't exceed SPM\_PAGESIZE (64 bytes on an ATtiny85), keeping app data intact if the master sends wrong page sizes. (Default: false).
* **CMD\_READDEVS**: Enables the READDEVS command, which reads all fuse bits, lock bits, and the device signature table. (Default: false).
* **EEPROM_ACCESS**: Enables the READEEPR and WRITEEPR commands to read and write the device EEPROM. (Default: false).
