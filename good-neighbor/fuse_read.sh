#!/bin/sh
echo ""
echo "*****************************"
echo "*                           *"
echo "* AVR microcontroller fuses *"
echo "* ========================= *"
echo "*                           *"
echo "*****************************"
echo ""
ARG1=${1:-fuse_settings.txt}
if avrdude -c USBasp -p attiny85 -B3 -U lfuse:r:lfuse.hex:h -U hfuse:r:hfuse.hex:h -U efuse:r:efuse.hex:h 2>>/dev/null;
then
	echo "AVR microcontroller fuses" > $ARG1;
	echo "=========================" >> $ARG1;
	echo "Low fuse = [`cat lfuse.hex`] <- Timonel default is 0x62" | tee -a $ARG1;
	echo "................................................." | tee -a $ARG1;
	echo "High fuse = [`cat hfuse.hex`] <- Timonel default is 0xdd" | tee -a $ARG1;
	echo "................................................." | tee -a $ARG1;
	echo "Extended fuse = [`cat efuse.hex`] <- Timonel default is 0xfe" | tee -a $ARG1;
	echo "................................................." | tee -a $ARG1;
	rm lfuse.hex hfuse.hex efuse.hex;
else
	echo "AVRdude execution error!";
fi
	