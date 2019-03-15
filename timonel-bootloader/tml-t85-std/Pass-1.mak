#
# File: Pass-1 (Makefile)
# Project: Timonel - I2C Bootloader for ATtiny85 MCUs
# Based on examples from "Make, AVR Programming" book
# .......................................................
# 2018-08-28 gustavo.casanova@nicebots.com
#

CONFIG ?= tml-t85-std

##########------------------------------------------------------##########
##########              Make Environment Setup                  ##########
##########------------------------------------------------------##########
CONFIGPATH = $(CONFIG)
include $(CONFIGPATH)/tml-config.mak

##########------------------------------------------------------##########
##########              Project-specific Details                ##########
##########    Check these every time you start a new project    ##########
##########------------------------------------------------------##########

# >>>>>> PASS 2 <<<<<<
#
# WE USE A TIMONEL START ADDRESS
# LOW ENOUGH TO ALLOW COMPILING
# EVEN WITH ALL FEATURES ENABLED

TIMONEL_START = $(LOW_FL)

## A directory for common include files and the simple USART library.
## If you move either the current folder or the Library folder, you'll 
##  need to change this path to match.

#LIBDIR = ../nb-libs/twis

##########------------------------------------------------------##########
##########                 Programmer Defaults                  ##########
##########          Set up once, then forget about it           ##########
##########        (Can override.  See bottom of file.)          ##########
##########------------------------------------------------------##########

PROGRAMMER_TYPE = usbasp
# extra arguments to avrdude: baud rate, chip type, -F flag, etc.
PROGRAMMER_ARGS = 	

##########------------------------------------------------------##########
##########                  Program Locations                   ##########
##########     Won't need to change if they're in your PATH     ##########
##########------------------------------------------------------##########

CC = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
AVRSIZE = avr-size
AVRDUDE = avrdude

##########------------------------------------------------------##########
##########                   Makefile Magic!                    ##########
##########         Summary:                                     ##########
##########             We want a .hex file                      ##########
##########        Compile source files into .elf                ##########
##########        Convert .elf file into .hex                   ##########
##########        You shouldn't need to edit below.             ##########
##########------------------------------------------------------##########

## Or name it automatically after the enclosing directory
#TARGET = $(lastword $(subst /, ,$(CURDIR)))

# Object files: will find all .c/.h files in current directory
#  and in LIBDIR.  If you have any other (sub-)directories with code,
#  you can add them in to SOURCES below in the wildcard statement.
#SOURCES=$(wildcard *.c *.cpp $(LIBDIR)/*.c $(LIBDIR)/*.cpp)
SOURCES=$(wildcard *.c $(LIBDIR)/*.c)
#SOURCES=$(wildcard *.c $(LIBDIR)/*.c $(LIBDIR)/*.S)
OBJECTS=$(SOURCES:.c=.o)
HEADERS=$(SOURCES:.c=.h)

## Compilation options, type man avr-gcc if you're curious.
#CPPFLAGS = -DF_CPU=$(F_CPU) -DBAUD=$(BAUD) -I. -I$(LIBDIR)
#CFLAGS = -Os -g -std=gnu99 -Wall
CFLAGS += -I. -g2 -Os -Wall -std=gnu99
## Use short (8-bit) data types 
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums 
## Splits up object files per function
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -nostartfiles -ffunction-sections -fdata-sections -fpack-struct -fno-inline-small-functions -fno-move-loop-invariants -fno-tree-scev-cprop

## Additional bootloader settings
CFLAGS += -I$(CONFIGPATH) -I$(LIBDIR) -I$(CMDDIR) -mmcu=$(MCU) -DF_CPU=$(F_CPU)
CFLAGS += -DTIMONEL_START=0x$(TIMONEL_START)
CFLAGS += -DTWI_ADDR=$(TIMONEL_TWI_ADDR)
CFLAGS += -DENABLE_LED_UI=$(ENABLE_LED_UI)
CFLAGS += -DAUTO_TPL_CALC=$(AUTO_TPL_CALC)
CFLAGS += -DAPP_USE_TPL_PG=$(APP_USE_TPL_PG)
CFLAGS += -DCMD_STPGADDR=$(CMD_STPGADDR)
CFLAGS += -DTWO_STEP_INIT=$(TWO_STEP_INIT)
CFLAGS += -DUSE_WDT_RESET=$(USE_WDT_RESET)
CFLAGS += -DCHECK_EMPTY_FL=$(CHECK_EMPTY_FL)
CFLAGS += -DCMD_READFLASH=$(CMD_READFLASH)
CFLAGS += -DLED_UI_PIN=$(LED_UI_PIN)
CFLAGS += -DCYCLESTOEXIT=$(CYCLESTOEXIT)
CFLAGS += -DSET_PRESCALER=$(SET_PRESCALER)
CFLAGS += -DFORCE_ERASE_PG=$(FORCE_ERASE_PG)

#LDFLAGS = -Wl,-Map,$(TARGET).map 
## Optional, but often ends up with smaller code
#LDFLAGS += -Wl,--gc-sections 
LDFLAGS += -Wl,--relax,--section-start=.text=$(TIMONEL_START),--gc-sections,-Map=$(TARGET).map
## Relax shrinks code even more, but makes disassembly messy
## LDFLAGS += -Wl,--relax
## LDFLAGS += -Wl,-u,vfprintf -lprintf_flt -lm  ## for floating-point printf
## LDFLAGS += -Wl,-u,vfprintf -lprintf_min      ## for smaller printf
TARGET_ARCH = -mmcu=$(MCU)

## Explicit pattern rules:
##  To make .o files from .c files 
#%.o: %.c $(HEADERS) Makefile
#	$(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c -o $@ $<;
 
$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LDLIBS) -o $@

%.hex: %.elf
	 $(OBJCOPY) -j .text -j .data -O ihex $< $@

%.eeprom: %.elf
	$(OBJCOPY) -j .eeprom --change-section-lma .eeprom=0 -O ihex $< $@ 

%.lst: %.elf
	$(OBJDUMP) -S $< > $@

## These targets don't have files named after them
.PHONY: all disassemble disasm eeprom size clean squeaky_clean flash fuses

all: $(TARGET).hex
	@echo
	@echo -----------------------------------------------
	@echo = Building Timonel configuration: $(CONFIG) =
	@echo -----------------------------------------------
	@echo
	@echo [Hexfile] Use the "data" size to calculate the bootloader address!
	@avr-size $(TARGET).hex

debug:
	@echo
	@echo "Source files:"   $(SOURCES)
	@echo "MCU, F_CPU, BAUD:"  $(MCU), $(F_CPU), $(BAUD)
	@echo	

# Optionally create listing file from .elf
# This creates approximate assembly-language equivalent of your code.
# Useful for debugging time-sensitive bits, 
# or making sure the compiler does what you want.
disassemble: $(TARGET).lst

disasm: disassemble

# Optionally show how big the resulting program is 
size:  $(TARGET).elf
	$(AVRSIZE) -C --mcu=$(MCU) $(TARGET).elf

clean:
	@rm $(TARGET).o
	@rm $(TARGET).map
	@rm $(TARGET).elf
	@rm $(LIBDIR)/nb-usitwisl-if.o

clean_all:
	@rm $(TARGET).o
	@rm $(TARGET).map
	@rm $(TARGET).elf
	@rm $(TARGET).hex
	@rm $(LIBDIR)/nb-usitwisl-if.o
    
squeaky_clean:
	rm -f *.elf *.hex *.obj *.o *.d *.eep *.lst *.lss *.sym *.map *~ *.eeprom

##########------------------------------------------------------##########
##########              Programmer-specific details             ##########
##########           Flashing code to AVR using avrdude         ##########
##########------------------------------------------------------##########

flash: $(TARGET).hex 
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(MCU) $(PROGRAMMER_ARGS) -U flash:w:$<

## An alias
program: flash

flash_eeprom: $(TARGET).eeprom
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(MCU) $(PROGRAMMER_ARGS) -U eeprom:w:$<

avrdude_terminal:
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(MCU) $(PROGRAMMER_ARGS) -nt

## If you've got multiple programmers that you use, 
## you can define them here so that it's easy to switch.
## To invoke, use something like `make flash_arduinoISP`
flash_usbtiny: PROGRAMMER_TYPE = usbtiny
flash_usbtiny: PROGRAMMER_ARGS =  # USBTiny works with no further arguments
flash_usbtiny: flash

flash_usbasp: PROGRAMMER_TYPE = usbasp
flash_usbasp: PROGRAMMER_ARGS =  # USBasp works with no further arguments
flash_usbasp: flash

flash_arduinoISP: PROGRAMMER_TYPE = avrisp
flash_arduinoISP: PROGRAMMER_ARGS = -b 19200 -P /dev/ttyACM0 
## (for windows) flash_arduinoISP: PROGRAMMER_ARGS = -b 19200 -P com5
flash_arduinoISP: flash

flash_109: PROGRAMMER_TYPE = avr109
flash_109: PROGRAMMER_ARGS = -b 9600 -P /dev/ttyUSB0
flash_109: flash

##########------------------------------------------------------##########
##########       Fuse settings and suitable defaults            ##########
##########------------------------------------------------------##########

## Mega 48, 88, 168, 328 default values
LFUSE = 0x62
HFUSE = 0xDD
EFUSE = 0xFE

## Generic 
FUSE_STRING = -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m 

fuses: 
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(MCU) \
	           $(PROGRAMMER_ARGS) $(FUSE_STRING)
show_fuses:
	$(AVRDUDE) -c $(PROGRAMMER_TYPE) -p $(MCU) $(PROGRAMMER_ARGS) -nv	

## Called with no extra definitions, sets to defaults
set_default_fuses:  FUSE_STRING = -U lfuse:w:$(LFUSE):m -U hfuse:w:$(HFUSE):m -U efuse:w:$(EFUSE):m 
set_default_fuses:  fuses

## Set the fuse byte for full-speed mode
## Note: can also be set in firmware for modern chips
set_fast_fuse: LFUSE = 0xE1
set_fast_fuse: FUSE_STRING = -U lfuse:w:$(LFUSE):m 
set_fast_fuse: fuses

## Set the EESAVE fuse byte to preserve EEPROM across flashes
set_eeprom_save_fuse: HFUSE = 0xDD
set_eeprom_save_fuse: FUSE_STRING = -U hfuse:w:$(HFUSE):m
set_eeprom_save_fuse: fuses

## Clear the EESAVE fuse byte
clear_eeprom_save_fuse: FUSE_STRING = -U hfuse:w:$(HFUSE):m
clear_eeprom_save_fuse: fuses
