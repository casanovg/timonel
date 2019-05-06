#
# File: Makefile
# Project: Timonel - TWI Bootloader
# ........................................................
# 2019-04-28 gustavo.casanova@nicebots.com
#

CONFIG ?= tml-t85-std

##########################################################
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

##########################################################
# Make command line parameters
# ----------------------------
# The below definitions are overwritten by
# the command line parameters, if exist.
ifeq ($(TARGET),)
	TARGET = timonel
endif

ifeq ($(TARGET),)
	TIMONEL_TWI_ADDR = 8
endif
# End of command line parameters
##########################################################

ifeq ($(TARGET),)
	TIMONEL_START = 1B00
endif

PROGRAMMER ?= -c USBasp
# PROGRAMMER contains AVRDUDE options to address your programmer

# Tools:
AVRDUDE = avrdude $(PROGRAMMER) -p $(MCU) 
CC = avr-gcc

# compiler options
CFLAGS += -Wall -g2 -Os -std=gnu99
CFLAGS += -I$(LIBDIR) -I$(CONFIGPATH) -I$(CMDDIR) -I. -mmcu=$(MCU) -DF_CPU=$(F_CPU)
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums 
## Splits up object files per function
CFLAGS += -nostartfiles -ffunction-sections -fdata-sections -fpack-struct
CFLAGS += -fno-inline-small-functions -fno-move-loop-invariants -fno-tree-scev-cprop

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

SOURCES=$(wildcard $(LIBDIR)/*.c $(CONFIGPATH)/*.S *.c)
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
	@echo ===================================================================
	@echo \| Building Timonel configuration: $(CONFIG)
	@echo \|------------------------------------------------------------------        
	@echo \| TIMONEL_START = 0x$(TIMONEL_START) \| TWI_ADDRESS = $(TIMONEL_TWI_ADDR) \| \"""$(TARGET).hex\"
	@echo ===================================================================

	@echo \| ... ENABLE_LED_UI = $(ENABLE_LED_UI)
	@echo \| ... AUTO_TPL_CALC = $(AUTO_TPL_CALC)
	@echo \| ... APP_USE_TPL_PG = $(APP_USE_TPL_PG)
	@echo \| ... CMD_STPGADDR = $(CMD_STPGADDR)
	@echo \| ... TWO_STEP_INIT = $(TWO_STEP_INIT)
	@echo \| ... USE_WDT_RESET = $(USE_WDT_RESET)
	@echo \| ... CHECK_EMPTY_FL = $(CHECK_EMPTY_FL)
	@echo \| ... CMD_READFLASH = $(CMD_READFLASH)
	@echo \|------------------------------------------------------------------    
	@echo \| ... LED_UI_PIN = $(LED_UI_PIN)
	@echo \| ... CYCLESTOEXIT = $(CYCLESTOEXIT)
	@echo \| ... SET_PRESCALER = $(SET_PRESCALER)
	@echo \| ... FORCE_ERASE_PG = $(FORCE_ERASE_PG)
	@echo -------------------------------------------------------------------    
	@rm -f $(TARGET).hex $(TARGET).eep.hex
	@avr-objcopy -j .text -j .data -O ihex $(TARGET).bin $(TARGET).hex
	@echo [Sections]
	@avr-size $(TARGET).bin
	@echo
	@echo [Hexfile] Use the "data" size to calculate the bootloader address!
	@avr-size $(TARGET).hex
	@echo -------------------------------------------------------------------     

upgrade: $(TARGET).bin
	avr-objcopy -O binary $(TARGET).bin $(TARGET).raw
	avr-objcopy -I binary -O elf32-avr \
        --rename-section .data=.text \
        --redefine-sym _binary_main_raw_start=loader \
        --redefine-sym _binary_main_raw_end=loader_end \
		$(TARGET).raw bootloader_linkable.o	
	
disasm:	$(TARGET).bin
	@avr-objdump -d -S $(TARGET).bin >$(TARGET).lss
