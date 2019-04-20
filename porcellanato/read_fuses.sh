#!/bin/sh
#echo ""
#echo "*****************************************"
#echo "*                                       *"
#echo "* Reading AVR microcontroller fuses ... *"
#echo "* ===================================== *"
#echo "*                                       *"
#echo "*****************************************"
#echo ""
ARG1=${1:-fuse_settings.txt}
avrdude -c USBasp -p attiny85 -B3 -U lfuse:r:lfuse.hex:h -U hfuse:r:hfuse.hex:h -U efuse:r:efuse.hex:h
echo "Low fuse: "`cat lfuse.hex` > $ARG1
echo "High fuse: "`cat hfuse.hex` >> $ARG1
echo "Extended fuse: "`cat efuse.hex` >> $ARG1
rm lfuse.hex hfuse.hex efuse.hex
