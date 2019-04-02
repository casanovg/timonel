#!/bin/sh
#echo ""
#echo "***********************************************************************"
#echo "*                                                                     *"
#echo "* Please use USBasp for flashing Timonel to the ATtiny device         *"
#echo "* =================================================================== *"
#echo "*                                                                     *"
#echo "***********************************************************************"
#echo ""

# 8 MHz
#avrdude -c USBasp -p attiny85 -U flash:w:.\\releases\\timonel.hex:i -B 20 -U lfuse:w:0x62:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m

# 16 MHz
#avrdude -c USBasp -p attiny85 -U flash:w:.\\releases\\timonel.hex:i -B 20 -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m
avrdude -c USBasp -p attiny85 -U flash:w:.\\releases\\porcellanato.hex:i -B 20 -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m
