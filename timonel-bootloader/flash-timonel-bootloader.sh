#!/bin/bash
#echo ""
#echo "***********************************************************************"
#echo "*                                                                     *"
#echo "* Please use USBasp for flashing Timonel to the ATtiny device         *"
#echo "* =================================================================== *"
#echo "* 2019-08-09 Gustavo Casanova                                         *"
#echo "***********************************************************************"
#echo ""

ARG1=${1:-timonel}
ARG2=${2:-1}
ARG3=${3:-usbasp}
ARG4=${4:-COM1}

FIRMWARE="./releases/$ARG1.hex";

HIGH_FUSE=0xD5;
EXTENDED_FUSE=0xFE;

case $ARG2 in
	16) # echo "";
		# echo "Low fuse set for sixteen MHz internal clock source.";
		LOW_FUSE=0xE1;
		;;
	8)  # echo "";
		# echo "Low fuse set for eight MHz internal clock source.";
		LOW_FUSE=0xE2;
		;;
	2)	# echo "";
		# echo "Low fuse set for two MHz internal clock source.";
		LOW_FUSE=0x61;
		;;
	1)	# echo "";
		# echo "Low fuse set for one MHz internal clock source.";
		LOW_FUSE=0x62;
		;;
	*)	echo ""
		echo $"Usage: $0 firmware {16|8|2|1}";
        exit 2;
esac

if [ -f "$FIRMWARE" ]
then
  echo "";
	echo "[[[ Flashing Timonel for operating @ $ARG2 MHz ]]]";

	case $ARG3 in
		avrdude | AVRdude | AVRDUDE)
			avrdude -c USBasp -p attiny85 -B3 -U flash:w:./releases/$ARG1.hex:i -B 20 -U lfuse:w:$LOW_FUSE:m -U hfuse:w:$HIGH_FUSE:m -U efuse:w:$EXTENDED_FUSE:m;
			;;
		stk500 | STK500)
			avrdude -c STK500 -P $ARG4 -p attiny85 -B3 -U flash:w:./releases/$ARG1.hex:i -B 20 -U lfuse:w:$LOW_FUSE:m -U hfuse:w:$HIGH_FUSE:m -U efuse:w:$EXTENDED_FUSE:m;
			;;
		*)
			avrdude -c USBasp -p attiny85 -B3 -U flash:w:./releases/$ARG1.hex:i -B 20 -U lfuse:w:$LOW_FUSE:m -U hfuse:w:$HIGH_FUSE:m -U efuse:w:$EXTENDED_FUSE:m;
	esac;		
else
	echo ""
	echo "WARNING: Firmware file \"$FIRMWARE\" not found, please check it!" >&2
	exit 1
fi
