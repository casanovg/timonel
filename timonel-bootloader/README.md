# Timonel Bootloader v1.6

What's new in this version:

* **Consistent little-endian byte order:** every multi-byte field in the protocol (memory addresses, page data, checksum accumulation) now sends LSB first, as agreed in [discussion #28](https://github.com/casanovg/timonel/discussions/28).
* **64-byte packets:** data packets are now officially 64 bytes (a full SPM page on an ATtiny85), up from 32.
* **More devices supported:** added configs for extra USI-based AVR families (ATtinyX4/X5, ATtiny2313, ATtinyX7, ATtiny26, ATtiny43U).
* **Smaller code:** use "|" instead of "+" when building flash addresses, saving 4 bytes each time (thanks to prandeamus).
* **Consistent "inline" usage:** fixed mismatches between declarations and definitions of helper functions (thanks to SMN321).
* **Better scripts:** STK500 programmer option with COM port support, "#!/bin/bash" shebang fixes, and path backslash fixes.
* **New config:** "tml-t85-test-comm" with features enabled for easy startup.

## Compilation

The **"make-timonel.sh"** script runs "[make](http://www.gnu.org/software/make)" with different configurations. It takes several arguments, including **"--all"**, which builds every config found in the **"configs"** folder. The flashable **".hex"** files are saved in the **"releases"** folder.

E.g: <b>`./make-timonel.sh tml-t85-small timonel 13 1B80 1 false;`</b>

* Builds a **"timonel.hex"** file based on the **"tml-t85-small"** config.
* Assigns TWI address **13** to the device in bootloader mode.
* Sets **0x1B80** as the bootloader start position.
* Sets the low fuse so the device runs at **1 MHz** in user-app mode.
* **Disables** automatic clock tweaking.

**Note:** this bootloader is compiled with the **"avr-gcc 8.3.0 64-bit"** toolchain from [this site](http://blog.zakkemble.net/avr-gcc-builds), which is also available in the "[avr-toolchains](http://github.com/casanovg/avr-toolchains)" repository. The scripts mostly exist to make flashing several devices less repetitive — you can always build and flash directly with avr-gcc and avrdude.

## <a id="Installation"></a>Flashing Timonel on the device

The **"flash-timonel-bootloader.sh"** script flashes Timonel with the "[avrdude](http://savannah.nongnu.org/projects/avrdude)" utility and an [AVR programmer](http://www.fischl.de/usbasp). The arguments are:

1. Binary file name (without the ".hex" extension), under the "releases" folder.
2. Clock speed in MHz: 16, 8, 2, 1.
The bootloader can be flashed to bare ATtiny85/45/25 chips, Digispark boards and other Tiny85-compatible devices with an AVR programmer (e.g. USBasp).

E.g: <b>`./flash-timonel-bootloader.sh timonel 1;`</b>

* Flashes the **"timonel.hex"** file to the device with avrdude.
* Sets the low fuse so the device runs at **1 MHz** (possible values are 1, 2, 8 and 16).

## Optional features

The bootloader has several optional features so you can find the right balance between features, flash usage, and performance. They're enabled from the **"tml-config.mak"** file inside each config folder (e.g. "tml-t85-std", the default one). It's a makefile include that adds or removes chunks of code from the final ".hex" binary. As a rule of thumb: more features = bigger bootloader = less room for user apps. Here's what's available:

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
