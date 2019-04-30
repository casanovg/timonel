# .......................................................
# File: tml-config.mak
# Project: Timonel - TWI Bootloader for TinyX5 MCUs
# .......................................................
# 2019-04-28 gustavo.casanova@nicebots.com
# .......................................................

# Microcontroller: ATtiny 85 - 1 MHz
# Configuration:   Standard

MCU = attiny85

# Hexadecimal address for bootloader section to begin. To calculate the best value:
# - make clean; make main.hex; ### output will list data: 2124 (or something like that)
# - for the size of your device (8kb = 1024 * 8 = 8192) subtract above value 2124... = 6068
# - How many pages in is that? 6068 / 64 (tiny85 page size in bytes) = 94.8125
# - round that down to 94 - our new bootloader address is 94 * 64 = 6016, in hex = 1780
# NOTE: If it doesn't compile, comment the below [# TIMONEL_START = XXXX ] line to

TIMONEL_START = 1B80

# Timonel TWI address (decimal value):
# -------------------------------------
# Allowed range: 8 to 35 (0x08 to 0x23)

TIMONEL_TWI_ADDR = 8

# Bootloader optional features:
# -----------------------------
# This options are commented in the "tmc-config.h" file

ENABLE_LED_UI  = true
AUTO_TPL_CALC  = true
APP_USE_TPL_PG = false
CMD_STPGADDR   = false
TWO_STEP_INIT  = true
USE_WDT_RESET  = true
CHECK_EMPTY_FL = false
CMD_READFLASH  = true
# Warning: By modifying the below options Timonel may become unresponsive!
LED_UI_PIN     = PB1
LOW_FUSE	   = 0x62
FORCE_ERASE_PG = false

# Project name:
# -------------
TARGET = timonel

# Timonel required libraries path:
# --------------------------------
#LIBDIR = ./nicebots-libs/twis/1-GC-if
#LIBDIR = ./nicebots-libs/twis/2-Blake
#LIBDIR = ./nicebots-libs/twis/3-AVR312
#LIBDIR = ./nicebots-libs/twis/4-eriksl
#LIBDIR = ./nicebots-libs/twis/5-RmbMst
LIBDIR = ./nicebots-libs/twis/6-GC-if-Good-Line
CMDDIR = ./nicebots-libs/cmd

# Settings for running at 1 Mhz starting from Timonel v1.1
FUSEOPT = -U lfuse:w:$(LOW_FUSE):m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m
FUSEOPT_DISABLERESET = -U lfuse:w:$(LOW:FUSE):m -U hfuse:w:0x5d:m -U efuse:w:0xfe:m

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
# 0xdd = 1 1 0 1   1 1 0 1
#        ^ ^ ^ ^   ^ \-+-/ 
#        | | | |   |   +------ BODLEVEL 2..0 (brownout trigger level -> 2.7V)
#        | | | |   +---------- EESAVE (preserve EEPROM on Chip Erase -> not preserved)
#        | | | +-------------- WDTON (watchdog timer always on -> disable)
#        | | +---------------- SPIEN (enable serial programming -> enabled)
#        | +------------------ DWEN (debug wire enable)
#        +-------------------- RSTDISBL (disable external reset -> enabled)
#
# Fuse high byte ("no reset": external reset disabled, can't program through SPI anymore):
# 0x5d = 0 1 0 1   1 1 0 1
#        ^ ^ ^ ^   ^ \-+-/ 
#        | | | |   |   +------ BODLEVEL 2..0 (brownout trigger level -> 2.7V)
#        | | | |   +---------- EESAVE (preserve EEPROM on Chip Erase -> not preserved)
#        | | | +-------------- WDTON (watchdog timer always on -> disable)
#        | | +---------------- SPIEN (enable serial programming -> enabled)
#        | +------------------ DWEN (debug wire enable)
#        +-------------------- RSTDISBL (disable external reset -> disabled!)
#
# Fuse low byte (default: 1 MHz):
# 0x62 = 0 1 1 0   0 0 1 0
#        ^ ^ \+/   \--+--/
#        | |  |       +------- CKSEL 3..0 (clock selection -> Int RF Oscillator)
#        | |  +--------------- SUT 1..0 (BOD enabled, fast rising power)
#        | +------------------ CKOUT (clock output on CKOUT pin -> disabled)
#        +-------------------- CKDIV8 (divide clock by 8 -> divide)
#
# Fuse low byte (16 MHz):
# 0xe1 = 1 1 1 0   0 0 0 1
#        ^ ^ \+/   \--+--/
#        | |  |       +------- CKSEL 3..0 (clock selection -> Int HF PLL)
#        | |  +--------------- SUT 1..0 (BOD enabled, fast rising power)
#        | +------------------ CKOUT (clock output on CKOUT pin -> disabled)
#        +-------------------- CKDIV8 (divide clock by 8 -> don't divide)

###############################################################################
