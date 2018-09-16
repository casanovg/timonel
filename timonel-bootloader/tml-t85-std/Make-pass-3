# File: Makefile
# Project: Timonel Bootloader
# Based on Makefile from micronucleus bootloader by Jenna Fox
# Adapter by Gustavo Casanova / Nicebots

CONFIG ?= tml-t85-std

###############################################################################
# Configure the following variables according to your AVR.
# Program the device with
#     make fuse    			# to set the clock generator, boot section size etc.
#     make flash   			# to load the boot loader into flash
#     make disablereset		# use external reset line for IO (CAUTION: this is not easy to enable again, see README)

## Project name (without the .c)
TARGET = tml-bootloader
## Automatic project name (after the enclosing directory)
#TARGET = $(lastword $(subst /, ,$(CURDIR)))

CFLAGS =
CONFIGPATH = $(CONFIG)
include $(CONFIGPATH)/Makefile.inc

# >>>>>> PASS 3 <<<<<<
#
# USE TIMONEL_START VALUE FROM Makefile.inc

LIBDIR = ./nb-libs

PROGRAMMER ?= -c USBasp
# PROGRAMMER contains AVRDUDE options to address your programmer

# Tools:
AVRDUDE = avrdude $(PROGRAMMER) -p $(MCU) 
CC = avr-gcc

# Options:
CFLAGS += -I. -g2 -Os -Wall
CFLAGS += -I$(CONFIGPATH) -I$(LIBDIR) -mmcu=$(MCU) -DF_CPU=$(F_CPU) -DTIMONEL_START=0x$(TIMONEL_START)
CFLAGS += -nostartfiles -ffunction-sections -fdata-sections -fpack-struct -fno-inline-small-functions -fno-move-loop-invariants -fno-tree-scev-cprop

#LDFLAGS = -Wl,--relax,--section-start=.text=$(TIMONEL_START),--gc-sections,-Map=$(TARGET).map,--cref,--section-start=.appvector=0x0
LDFLAGS = -Wl,--relax,--section-start=.text=$(TIMONEL_START),--gc-sections,-Map=$(TARGET).map

OBJECTS = tml-bootloader.o $(LIBDIR)/nb-usitwisl-if.o $(LIBDIR)/crt1.o

# symbolic targets:
all: $(TARGET).hex

.c.o:
	@$(CC) $(CFLAGS) -c $< -o $@ -Wa,-ahls=$<.lst

.S.o:
	@$(CC) $(CFLAGS) -x assembler-with-cpp -c $< -o $@
# "-x assembler-with-cpp" should not be necessary since this is the default
# file type for the .S (with capital S) extension. However, upper case
# characters are not always preserved on Windows. To ensure WinAVR
# compatibility define the file type manually.

.c.s:
	@$(CC) $(CFLAGS) -S $< -o $@

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
	@rm -f $(TARGET).hex $(TARGET).bin $(TARGET).c.lst $(TARGET).map $(TARGET).raw $(TARGET).s $(TARGET).lss $(TARGET).lst $(TARGET).o $(TARGET).elf $(LIBDIR)/*.lst $(LIBDIR)/*.o	

# file targets:
$(TARGET).bin:	$(OBJECTS)
	@$(CC) $(CFLAGS) -o $(TARGET).bin $(OBJECTS) $(LDFLAGS)

$(TARGET).hex:	$(TARGET).bin
	@echo
	@echo -----------------------------------------------
	@echo = Building Timonel configuration: $(CONFIG) =
	@echo -----------------------------------------------
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
