#!/bin/sh
#echo ""
#echo "***********************************************************************"
#echo "*                                                                     *"
#echo "* Please use USBasp for flashing Timonel to the ATtiny device         *"
#echo "* =================================================================== *"
#echo "*                                                                     *"
#echo "***********************************************************************"
#echo ""

ARG1=${1:-timonel}
ARG2=${1:-8MHz}

FIRMWARE=".\\releases\\$ARG1.hex"

clear;

if [ -f "$FIRMWARE" ]
then
    echo ""
	if [ "$ARG2" != "16MHz" ]
	then
		# 8 MHz
		echo "[[[ Flashing Timonel for 8 MHz operation ]]]";
		avrdude -c USBasp -p attiny85 -B3 -U flash:w:.\\releases\\$ARG1.hex:i -B 20 -U lfuse:w:0x62:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m;
	else
		# 16 MHz
		echo "[[[ Flashing Timonel for 16 MHz operation ]]]";
		avrdude -c USBasp -p attiny85 -B3 -U flash:w:.\\releases\\$ARG1.hex:i -B 20 -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m;
	fi
else
	echo ""
	echo "WARNING: Firmware file \"$FIRMWARE\" not found, please check it!" >&2
fi


