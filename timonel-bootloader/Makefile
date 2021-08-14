#
# File: Makefile
# Project: Timonel - TWI Bootloader
# ........................................................
# 2020-08-09 gustavo.casanova@gmail.com
#

CONFIG ?= tml-t85-std
CFG_DIR ?= configs

##########################################################
# Configure the following variables according to your AVR.
# Program the device with
#     make fuse    			# to set the clock generator, boot section size etc.
#     make flash   			# to load the boot loader into flash
#     make disablereset		# use external reset line for IO (CAUTION: this is not easy to enable again!)

## Automatic project name (after the enclosing directory)
#TARGET = $(lastword $(subst /, ,$(CURDIR)))

CFLAGS =
CONFIGPATH = $(CFG_DIR)/$(CONFIG)
include $(CONFIGPATH).mak

##########################################################
# Make command line parameters
# ----------------------------
# The below definitions are overwritten by
# the command line parameters, if exist.
ifeq ($(TARGET),)
	TARGET = timonel
endif

ifeq ($(TIMONEL_TWI_ADDR),)
	TIMONEL_TWI_ADDR = 14
endif

ifeq ($(TIMONEL_START),)
	TIMONEL_START = 1B00
endif

ifeq ($(LOW_FUSE),)
	LOW_FUSE = 0x62
endif
# End of command line parameters
##########################################################

PROGRAMMER ?= -c USBasp
# PROGRAMMER contains AVRDUDE options to address your programmer

# Tools:
AVRDUDE = avrdude $(PROGRAMMER) -p $(MCU) 
CC = avr-gcc
#CC = /usr/local/bin/avr-7.2.0/bin/avr-gcc

# Compiler options
CFLAGS += -Wall -g2 -Os -std=gnu99
# The "LIBDIR" path is required when including an external TWI driver, deprecated from v1.4 onwards
# CFLAGS += -I$(LIBDIR)
CFLAGS += -I$(CONFIGPATH) -I$(CMDDIR) -I. -mmcu=$(MCU) -DF_CPU=$(F_CPU)
## Splits up object files per function
CFLAGS += -ffunction-sections -fdata-sections
# The "nostartfiles" option allows ditching the compiler-supplied "crt1.S" file to include a custom one
CFLAGS += -nostartfiles
# Optimizations aimed at reducing memory space usage
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums -mno-interrupts -mtiny-stack
CFLAGS += -fno-inline-small-functions -fno-move-loop-invariants -fno-tree-scev-cprop -fno-jump-tables
# Bootloader TWI address and memory start position
CFLAGS += -DTWI_ADDR=$(TIMONEL_TWI_ADDR)
CFLAGS += -DTIMONEL_START=0x$(TIMONEL_START)
# Bootloader optional feature selection
CFLAGS += -DENABLE_LED_UI=$(ENABLE_LED_UI)
CFLAGS += -DAUTO_PAGE_ADDR=$(AUTO_PAGE_ADDR)
CFLAGS += -DAPP_USE_TPL_PG=$(APP_USE_TPL_PG)
CFLAGS += -DCMD_SETPGADDR=$(CMD_SETPGADDR)
CFLAGS += -DTWO_STEP_INIT=$(TWO_STEP_INIT)
CFLAGS += -DUSE_WDT_RESET=$(USE_WDT_RESET)
CFLAGS += -DAPP_AUTORUN=$(APP_AUTORUN)
CFLAGS += -DCMD_READFLASH=$(CMD_READFLASH)
CFLAGS += -DCMD_READDEVS=$(CMD_READDEVS)
CFLAGS += -DEEPROM_ACCESS=$(EEPROM_ACCESS)
# Bootloader additional features
CFLAGS += -DAUTO_CLK_TWEAK=$(AUTO_CLK_TWEAK)
CFLAGS += -DLOW_FUSE=$(LOW_FUSE)
CFLAGS += -DLED_UI_PIN=$(LED_UI_PIN)
# Linker options
LDFLAGS = -Wl,--relax,--section-start=.text=$(TIMONEL_START),--gc-sections,-Map=$(TARGET).map

SOURCES=$(wildcard $(LIBDIR)/*.c *.S *.c)
OBJECTS=$(SOURCES:.c=.o)
HEADERS=$(SOURCES:.c=.h)

# symbolic targets:
all: $(TARGET).hex

#%.o: %.c $(HEADERS)
#$(TARGET).o: %.c $(HEADERS) $(TARGET)
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
	@rm -f $(TARGET).hex $(TARGET).bin $(TARGET).c.lst $(TARGET).map $(TARGET).raw $(TARGET).s \
	$(TARGET).lss $(TARGET).lst $(TARGET).o $(TARGET).elf $(LIBDIR)/*.lst

clean_all:
	@rm -f $(TARGET).hex $(TARGET).bin $(TARGET).c.lst $(TARGET).map $(TARGET).raw $(TARGET).s \
	$(TARGET).lss $(TARGET).lst $(TARGET).o $(TARGET).elf $(LIBDIR)/*.lst $(LIBDIR)/*.o *.o *.c.lst

# file targets:
$(TARGET).bin:	$(OBJECTS)
	@$(CC) $(CFLAGS) -o $(TARGET).bin $(OBJECTS) $(LDFLAGS)

$(TARGET).elf: $(OBJECTS)
	$(CC) $(LDFLAGS) -mmcu=$(MCU) $^ $(LDLIBS) -o $@	

$(TARGET).hex:	$(TARGET).bin

	@echo 
	@echo ========================================================================
	@echo \| Building Timonel configuration: $(CONFIG)
	@echo \|-----------------------------------------------------------------------
	@echo \| TIMONEL_START = 0x$(TIMONEL_START) \| TWI_ADDRESS = $(TIMONEL_TWI_ADDR) \| \"""$(TARGET).hex\"
	@echo ========================================================================
	@echo \| ... ENABLE_LED_UI = $(ENABLE_LED_UI)
	@echo \| ... AUTO_PAGE_ADDR = $(AUTO_PAGE_ADDR)
	@echo \| ... APP_USE_TPL_PG = $(APP_USE_TPL_PG)
	@echo \| ... CMD_SETPGADDR = $(CMD_SETPGADDR)
	@echo \| ... TWO_STEP_INIT = $(TWO_STEP_INIT)
	@echo \| ... USE_WDT_RESET = $(USE_WDT_RESET)
	@echo \| ... APP_AUTORUN = $(APP_AUTORUN)
	@echo \| ... CMD_READFLASH = $(CMD_READFLASH)
	@echo \| ... CMD_READDEVS = $(CMD_READDEVS)
	@echo \| ... EEPROM_ACCESS = $(EEPROM_ACCESS)
	@echo \|-----------------------------------------------------------------------
	@echo \| ... AUTO_CLK_TWEAK = $(AUTO_CLK_TWEAK)
	@echo \| ... LOW_FUSE = $(LOW_FUSE)	
	@echo \| ... LED_UI_PIN = $(LED_UI_PIN)
	@echo ------------------------------------------------------------------------
	@rm -f $(TARGET).hex $(TARGET).eep.hex
	@avr-objcopy -j .text -j .data -O ihex $(TARGET).bin $(TARGET).hex
	@echo [Sections]
	@avr-size $(TARGET).bin
	@echo
	@echo [Hexfile] Take \"data\" size to calculate the bootloader\'s start address!
	@avr-size $(TARGET).hex
	@echo ------------------------------------------------------------------------

upgrade: $(TARGET).bin
	avr-objcopy -O binary $(TARGET).bin $(TARGET).raw
	avr-objcopy -I binary -O elf32-avr \
        --rename-section .data=.text \
        --redefine-sym _binary_main_raw_start=loader \
        --redefine-sym _binary_main_raw_end=loader_end \
		$(TARGET).raw bootloader_linkable.o	
	
disasm:	$(TARGET).bin
	@avr-objdump -d -S $(TARGET).bin >$(TARGET).lss
