#!/bin/sh
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

FIRMWARE="./releases/$ARG1.hex";

HIGH_FUSE=0xDD;
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
	avrdude -c USBasp -p attiny85 -B3 -U flash:w:.\\releases\\$ARG1.hex:i -B 20 -U lfuse:w:$LOW_FUSE:m -U hfuse:w:$HIGH_FUSE:m -U efuse:w:$EXTENDED_FUSE:m;
else
	echo ""
	echo "WARNING: Firmware file \"$FIRMWARE\" not found, please check it!" >&2
	exit 1
fi

# if [ -f "$FIRMWARE" ]
# then
    # echo ""
	# if [ "$ARG2" != "16" ];
	# then 
		# # 8 MHz
		# echo "[[[ Flashing Timonel for operating @ 8 MHz ]]]";
		# echo "";
		# avrdude -c USBasp -p attiny85 -B3 -U flash:w:.\\releases\\$ARG1.hex:i -B 20 -U lfuse:w:0x62:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m;
	# else
		# # 16 MHz
		# echo "[[[ Flashing Timonel for operating @ 16 MHz ]]]";
		# echo "";
		# avrdude -c USBasp -p attiny85 -B3 -U flash:w:.\\releases\\$ARG1.hex:i -B 20 -U lfuse:w:0xe1:m -U hfuse:w:0xdd:m -U efuse:w:0xfe:m;
	# fi;
# else
	# echo ""
	# echo "WARNING: Firmware file \"$FIRMWARE\" not found, please check it!" >&2
# fi

