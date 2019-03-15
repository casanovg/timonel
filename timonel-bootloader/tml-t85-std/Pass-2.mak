#
# File: Pass-2 (Makefile)
# Project: Timonel - I2C Bootloader for ATtiny85 MCUs
# Based on Makefile from micronucleus project
# .......................................................
# 2018-08-28 gustavo.casanova@nicebots.com
#

CONFIG ?= tml-t85-std

###############################################################################
# Configure the following variables according to your AVR.
# Program the device with
#     make fuse    			# to set the clock generator, boot section size etc.
#     make flash   			# to load the boot loader into flash
#     make disablereset		# use external reset line for IO (CAUTION: this is not easy to enable again, see README)

## Automatic project name (after the enclosing directory)
#TARGET = $(lastword $(subst /, ,$(CURDIR)))

CFLAGS =
CONFIGPATH = $(CONFIG)
include $(CONFIGPATH)/tml-config.mak

# >>>>>> PASS 3 <<<<<<
#
# WE USE THE ORIGINAL TIMONEL_START
# ADDRESS DEFINED IN "Makefile.inc"
###################################

PROGRAMMER ?= -c USBasp
# PROGRAMMER contains AVRDUDE options to address your programmer

# Tools:
AVRDUDE = avrdude $(PROGRAMMER) -p $(MCU) 
CC = avr-gcc

# Additional bootloader settings:
CFLAGS += -I. -I$(CMDDIR) -g2 -Os -Wall -std=gnu99

CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums 
## Splits up object files per function
CFLAGS += -ffunction-sections -fdata-sections 
CFLAGS += -nostartfiles -ffunction-sections -fdata-sections -fpack-struct -fno-inline-small-functions -fno-move-loop-invariants -fno-tree-scev-cprop

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

LDFLAGS = -Wl,--relax,--section-start=.text=$(TIMONEL_START),--gc-sections,-Map=$(TARGET).map

#OBJECTS = $(TARGET).o $(LIBDIR)/nb-usitwisl-if.o $(LIBDIR)/crt1.o

SOURCES=$(wildcard *.c $(LIBDIR)/*.c $(LIBDIR)/*.S)
OBJECTS=$(SOURCES:.c=.o)
HEADERS=$(SOURCES:.c=.h)

# symbolic targets:
all: $(TARGET).hex

#.c.o:
#%.o: %.c $(HEADERS)
#	@$(CC) $(CFLAGS) -c $< -o $@ -Wa,-ahls=$<.lst

.S.o:
	@$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

#.c.s:
#	@$(CC) $(CFLAGS) -S $< -o $@

flash:	all
	$(AVRDUDE) -U flash:w:$(TARGET).hex:i -B 20

readflash:
	$(AVRDUDE) -U flash:r:flashdump.hex:i -B 20

fuse:
	$(AVRDUDE) $(FUSEOPT) -B 20
	
disablereset:
	$(AVRDUDE) $(FUSEOPT_DISABLERESET) -B 20

read_fuses:
	$(UISP) --rd_fuses -B 20

clean:
	@rm -f $(TARGET).hex $(TARGET).bin $(TARGET).c.lst $(TARGET).map $(TARGET).raw $(TARGET).s $(TARGET).lss $(TARGET).lst $(TARGET).o $(TARGET).elf $(LIBDIR)/*.lst

clean_all:
	@rm -f $(TARGET).hex $(TARGET).bin $(TARGET).c.lst $(TARGET).map $(TARGET).raw $(TARGET).s $(TARGET).lss $(TARGET).lst $(TARGET).o $(TARGET).elf $(LIBDIR)/*.lst $(LIBDIR)/*.o *.o	

# file targets:
$(TARGET).bin:	$(OBJECTS)
	@$(CC) $(CFLAGS) -o $(TARGET).bin $(OBJECTS) $(LDFLAGS)

$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -mmcu=$(MCU) $^ $(LDLIBS) -o $@	

$(TARGET).hex:	$(TARGET).bin
	@echo
	@echo
	@echo ===============================================
	@echo = Building Timonel configuration: $(CONFIG) =
	@echo ===============================================
	@rm -f $(TARGET).hex $(TARGET).eep.hex
	@avr-objcopy -j .text -j .data -O ihex $(TARGET).bin $(TARGET).hex
	@echo
	@echo [Sections]
	@avr-size $(TARGET).bin	
	@echo
	@echo [Hexfile] Use the "data" size to calculate the bootloader address!
	@avr-size $(TARGET).hex

upgrade: $(TARGET).bin
	avr-objcopy -O binary $(TARGET).bin $(TARGET).raw
	avr-objcopy -I binary -O elf32-avr \
        --rename-section .data=.text \
        --redefine-sym _binary_main_raw_start=loader \
        --redefine-sym _binary_main_raw_end=loader_end \
		$(TARGET).raw bootloader_linkable.o	
	
disasm:	$(TARGET).bin
	@avr-objdump -d -S $(TARGET).bin >$(TARGET).lss
