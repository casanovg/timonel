# .......................................................
# File: tml-config.mak
# Project: Timonel - TWI Bootloader for TinyX313 MCUs
# .......................................................
# 2021-07-25 gustavo.casanova@gmail.com
# .......................................................

# Microcontroller: ATtiny2313 @ 1 MHz
# Configuration:   Small: Only CMD_SETPGADDR enabled

MCU = attiny2313

# Hexadecimal address for bootloader section to begin. To calculate the best value:
# - ./make-timonel.sh tml-t2313-small tml-t2313-small 14 4c0 1 false
### output will list data: 814 (or something like that)
# - for the size of your device (2Kb = 1024 * 2 = 2048) subtract above value 814... = 1234
# - How many pages in is that? 1234 / 32 (tiny2323A page size in bytes) = 38.5625
# - round that down to 38 - our new bootloader address is 38 * 32 = 1216, in hex = 4c0
# - NOTE: If make-timonel.sh fails, try a low start address (e.g. 300 hex),
#         then do the calculation mentioned above.

TIMONEL_START = 4c0

# Timonel TWI address (decimal value):
# -------------------------------------
# Allowed range: 8 to 35 (0x08 to 0x23)

TIMONEL_TWI_ADDR = 14

# Bootloader optional features:
# -----------------------------
# These options are commented in the "tmc-config.h" file

ENABLE_LED_UI  = false
AUTO_PAGE_ADDR = false
APP_USE_TPL_PG = false
CMD_SETPGADDR  = true
TWO_STEP_INIT  = false
USE_WDT_RESET  = false
APP_AUTORUN    = false
CMD_READFLASH  = false
CMD_READDEVS   = false
EEPROM_ACCESS  = false
# Warning: Please modify the below options with caution ...
AUTO_CLK_TWEAK = false
LOW_FUSE       = 0x64
LED_UI_PIN     = PB1

# Project name:
# -------------
TARGET = timonel

# Timonel required libraries path:
# --------------------------------
#LIBDIR = ../../nb-libs/twis
CMDDIR = ../../nb-libs/cmd

# Settings for running at 1 Mhz starting from Timonel v1.1
FUSEOPT = -U lfuse:w:$(LOW_FUSE):m -U hfuse:w:0x9b:m -U efuse:w:0xfe:m
FUSEOPT_DISABLERESET = -U lfuse:w:$(LOW_FUSE):m -U hfuse:w:0x9a:m -U efuse:w:0xfe:m

#---------------------------------------------------------------------
# ATtiny85
#---------------------------------------------------------------------
# Fuse extended byte:
# 0xFE = - - - -   - 1 1 0
#                        ^
#                        |
#                        +---- SELFPRGEN (enable self programming flash)
#
# Fuse high byte (default):
# 0x9b = 1 0 0 1   1 0 1 1
#        ^ ^ ^ ^   \-+-/ ^
#        | | | |     |   +---- RSTDISBL (disable external reset -> disabled)
#        | | | |     +-------- BODLEVEL 2..0 (brownout trigger level -> 2.7V)
#        | | | +-------------- WDTON (watchdog timer always on -> disabled)
#        | | +---------------- SPIEN (enable serial programming -> enabled)
#        | +------------------ EESAVE (preserve EEPROM on Chip Erase -> preserved)
#        +-------------------- DWEN (debug wire -> disabled)
#
# Fuse high byte ("no reset": external reset disabled, can't program through SPI anymore):
# 0x9a = 1 0 0 1   1 0 1 0
#        ^ ^ ^ ^   \-+-/ ^ 
#        | | | |     |   +---- RSTDISBL (disable external reset -> WARNING! enabled)
#        | | | |     +-------- BODLEVEL 2..0 (brownout trigger level -> 2.7V)
#        | | | +-------------- WDTON (watchdog timer always on -> disable)
#        | | +---------------- SPIEN (enable serial programming -> enabled)
#        | +------------------ EESAVE (preserve EEPROM on Chip Erase -> preserved)
#        +-------------------- DWEN (debug wire enable)
#
# Fuse low byte (default: 1 MHz):
# 0x64 = 0 1 1 0   0 1 0 0
#        ^ ^ \+/   \--+--/
#        | |  |       +------- CKSEL 3..0 (clock selection -> Int RF Oscillator)
#        | |  +--------------- SUT 1..0 (BOD enabled, slowly rising power)
#        | +------------------ CKOUT (clock output on CKOUT pin -> disabled)
#        +-------------------- CKDIV8 (divide clock by 8 -> divide)
#
# Fuse low byte (8 MHz):
# 0xe4 = 1 1 1 0   0 1 0 0
#        ^ ^ \+/   \--+--/
#        | |  |       +------- CKSEL 3..0 (clock selection -> Int RF Oscillator)
#        | |  +--------------- SUT 1..0 (BOD enabled, slowly rising power)
#        | +------------------ CKOUT (clock output on CKOUT pin -> disabled)
#        +-------------------- CKDIV8 (divide clock by 8 -> don't divide)

###############################################################################
