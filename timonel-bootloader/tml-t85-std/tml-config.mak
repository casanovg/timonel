# .......................................................
# File: tml-config.mak
# Project: Timonel - TWI Bootloader for TinyX5 MCUs
# .......................................................
# 2018-08-28 gustavo.casanova@nicebots.com
# .......................................................

# Controller type: ATtiny 85 - 16.5 MHz
# Configuration:   Standard

F_CPU = 16500000
MCU = attiny85

# Hexadecimal address for bootloader section to begin. To calculate the best value:
# - make clean; make main.hex; ### output will list data: 2124 (or something like that)
# - for the size of your device (8kb = 1024 * 8 = 8192) subtract above value 2124... = 6068
# - How many pages in is that? 6068 / 64 (tiny85 page size in bytes) = 94.8125
# - round that down to 94 - our new bootloader address is 94 * 64 = 6016, in hex = 1780
# NOTE: If it doesn't compile, comment the below [# TIMONEL_START = XXXX ] line to

TIMONEL_START = 1B00

# Timonel TWI address (decimal value):
# -------------------------------------
# Allowed range: 8 to 35 (0x08 to 0x23)

TIMONEL_TWI_ADDR = 8

# Bootloader optional features:
# -----------------------------
# This options are commented in the "tmc-config.h" file

ENABLE_LED_UI  = false
AUTO_TPL_CALC  = true
APP_USE_TPL_PG = false
CMD_STPGADDR   = false
TWO_STEP_INIT  = false
USE_WDT_RESET  = true
CHECK_EMPTY_FL = false
CMD_READFLASH  = false
# Warning: By modifying the below options Timonel may become unresponsive!
LED_UI_PIN     = PB1
CYCLESTOEXIT   = 40
SET_PRESCALER  = true
FORCE_ERASE_PG = false

# Timonel required libraries path:
# --------------------------------
LIBDIR = ../nb-libs/twis
CMDDIR = ../nb-libs/cmd
# Project name:
# -------------
TARGET = timonel

# This is a flash address low enough to allow
# compiling in Pass 1, even with all features
# enabled. Please don't change it.
LOW_FL = 1500

# Settings for running at 8 Mhz starting from Timonel v1.1
FUSEOPT = -U lfuse:w:0x62:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m
FUSEOPT_DISABLERESET = -U lfuse:w:0x62:m -U hfuse:w:0x5d:m -U efuse:w:0xfe:m

#---------------------------------------------------------------------
# ATtiny85
#---------------------------------------------------------------------
# Fuse extended byte:
# 0xFE = - - - -   - 1 1 0
#                        ^
#                        |
#                        +---- SELFPRGEN (enable self programming flash)
#
# Fuse high byte:
# 0xdd = 1 1 0 1   1 1 0 1
#        ^ ^ ^ ^   ^ \-+-/ 
#        | | | |   |   +------ BODLEVEL 2..0 (brownout trigger level -> 2.7V)
#        | | | |   +---------- EESAVE (preserve EEPROM on Chip Erase -> not preserved)
#        | | | +-------------- WDTON (watchdog timer always on -> disable)
#        | | +---------------- SPIEN (enable serial programming -> enabled)
#        | +------------------ DWEN (debug wire enable)
#        +-------------------- RSTDISBL (disable external reset -> enabled)
#
# Fuse high byte ("no reset": external reset disabled, can't program through SPI anymore)
# 0x5d = 0 1 0 1   1 1 0 1
#        ^ ^ ^ ^   ^ \-+-/ 
#        | | | |   |   +------ BODLEVEL 2..0 (brownout trigger level -> 2.7V)
#        | | | |   +---------- EESAVE (preserve EEPROM on Chip Erase -> not preserved)
#        | | | +-------------- WDTON (watchdog timer always on -> disable)
#        | | +---------------- SPIEN (enable serial programming -> enabled)
#        | +------------------ DWEN (debug wire enable)
#        +-------------------- RSTDISBL (disable external reset -> disabled!)
#
# Fuse low byte:
# 0xe1 = 1 1 1 0   0 0 0 1
#        ^ ^ \+/   \--+--/
#        | |  |       +------- CKSEL 3..0 (clock selection -> HF PLL)
#        | |  +--------------- SUT 1..0 (BOD enabled, fast rising power)
#        | +------------------ CKOUT (clock output on CKOUT pin -> disabled)
#        +-------------------- CKDIV8 (divide clock by 8 -> don't divide)

###############################################################################
