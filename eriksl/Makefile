MCU			=		attiny85
PROGRAMMER	=		dragon_isp
PRGFLAGS	=		-b 0 -P usb

PROGRAM		=		twimain
OBJFILES	=		adc_temp.o ioports.o timer0_simple.o usitwislave/usitwislave.o $(PROGRAM).o
HEADERS		=		adc_temp.h ioports.h timer0_simple.h usitwislave/usitwislave_devices.h usitwislave/usitwislave.h
HEXFILE		=		$(PROGRAM).hex
ELFFILE		=		$(PROGRAM).elf
PROGRAMMED	=		.programmed
CFLAGS		=		-Wall -Winline -O3 -mmcu=$(MCU) -DF_CPU=8000000UL -Iusitwislave \
					-fpack-struct -funroll-loops -funit-at-a-time -fno-keep-static-consts \
					-frename-registers
LDFLAGS		=		-Wall -mmcu=$(MCU)

.PHONY:				all clean hex
.SUFFIXES:
.SUFFIXES:			.c .o .elf .hex
.PRECIOUS:			.c .h

all:				$(PROGRAMMED)
hex:				$(HEXFILE)

$(PROGRAM).o:		$(PROGRAM).c $(HEADERS)

%.o:				%.c
					@echo "CC $< -> $@"
					@avr-gcc -c $(CFLAGS) $< -o $@

%.s:				%.c
					@echo "CC (ASM) $< -> $@"
					@avr-gcc -S $(CFLAGS) $< -o $@

adc_temp.o:					adc_temp.h
ioports.o:					ioports.h
timer0_simple.o:			timer0_simple.h
usitwislave/usitwislave.o:	./usitwislave/usitwislave.h ./usitwislave/usitwislave_devices.h 

$(ELFFILE):			$(OBJFILES)
					@echo "LD $< -> $@"
					@avr-gcc $(LDFLAGS) $(OBJFILES) -o $@

$(HEXFILE):			$(ELFFILE)
					@echo "OBJCOPY $< -> $@"
					@avr-objcopy -j .text -j .data -O ihex $< $@
					@sh -c 'avr-size $< | (read header; read text data bss junk; echo "SIZE: flash: $$[text + data] ram: $$[data + bss]")'

$(PROGRAMMED):		$(HEXFILE)
					@echo "AVRDUDE $^"
					@sh -c "avrdude -vv -c $(PROGRAMMER) -p $(MCU) $(PRGFLAGS) -U flash:w:$^ > $(PROGRAMMED) 2>&1"

clean:			
					@echo "RM $(OBJFILES) $(ELFFILE) $(HEXFILE) $(PROGRAMMED)"
					@-rm $(OBJFILES) $(ELFFILE) $(HEXFILE) 2> /dev/null || true
