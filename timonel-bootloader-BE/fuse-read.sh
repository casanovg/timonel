#!/bin/bash
echo ""
echo "****************************************"
echo "*                                      *"
echo "*  AVR microcontroller fuses settings  *"
echo "*  ..................................  *"
echo "*  2018-10-20 Gustavo Casanova         *"
echo "*                                      *"
echo "****************************************"
echo ""
ARG1=${1:-fuse-settings.txt}
LF="0x62"
HF="0xd5"
EF="0xfe"
if avrdude -c USBasp -p attiny85 -B3 -U lfuse:r:lfuse.hex:h -U hfuse:r:hfuse.hex:h -U efuse:r:efuse.hex:h 2>>/dev/null;
then
	echo "AVR microcontroller fuses" > $ARG1;
	echo "=========================" >> $ARG1;
    if [ "`cat lfuse.hex`" == $LF ]; then
        echo "Low fuse = [$LF] <- OK: default setting" | tee -a $ARG1;
    else
        echo "Low fuse = [`cat lfuse.hex`] <- Timonel default is $LF" | tee -a $ARG1;
    fi;
    echo "................................................." | tee -a $ARG1;
    if [ "`cat hfuse.hex`" == $HF ]; then
        echo "High fuse = [$HF] <- OK: default setting" | tee -a $ARG1;
    else
        echo "High fuse = [`cat hfuse.hex`] <- Timonel default is $HF" | tee -a $ARG1;
    fi;
    echo "................................................." | tee -a $ARG1;
    if [ "`cat efuse.hex`" == $EF ]; then
        echo "Extended fuse = [$EF] <- OK: default setting" | tee -a $ARG1;
    else
        echo "Extended fuse = [`cat efuse.hex`] <- Timonel default is $EF" | tee -a $ARG1;
    fi;
    echo "................................................." | tee -a $ARG1;
	rm lfuse.hex hfuse.hex efuse.hex;
else
	echo "AVRdude execution error!";
fi
	