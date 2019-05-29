#!/bin/sh
#############################################################
# MAKE_TIMONEL.sh                                           #
# ......................................................... #
# This script generates Timonel bootloader images that are  #
# flashable with an AVR programmer (e.g. USBasp).           #
#                                                           #
# ......................................................... #
# 2019-06-06 Gustavo Casanova                               #
# ......................................................... #
# From Linux: it should run directly.                       #
# From Windows/Mac: you need to install the Git Bash tool   #
# for your system (git-scm.com/downloads).                  #
#                                                           #
#############################################################

# Command line arguments
# ----------------------
# ARG1: Timonel .hex filename. Default = timonel
# ARG2: Timonel TWI (I2C) address. Default = 11
# ARG3: Timonel start memory position. Default = 1B80 (hex)
# ARG4: LOW_FUSE clock adjustments. Valid options = 16, 8, 2, 1. Default = 1 (1 MHz)
# ARG5: Automatic clock tweaking. When is enabled, it overrides LOW_FUSE value. Default = false

ARG1=${1:-timonel}
ARG2=${2:-11}
ARG3=${3:-1B80}
ARG4=${4:-1}
ARG5=${5:-false}

case $ARG4 in
    16)
        # echo "";
        # echo "Low fuse set for sixteen MHz internal clock source.";
        LOW_FUSE=0xE1;
        CLK_SOURCE="HF PLL";
        ;;
    8)
        # echo "";
        # echo "Low fuse set for eight MHz internal clock source.";
        LOW_FUSE=0xE2;
        CLK_SOURCE="RC OSC";
        ;;
    2)
        # echo "";
        # echo "Low fuse set for two MHz internal clock source.";
        LOW_FUSE=0x61;
        CLK_SOURCE="HF PLL";
        ;;
    1)
        # echo "";
        # echo "Low fuse set for one MHz internal clock source.";
        LOW_FUSE=0x62;
        CLK_SOURCE="RC OSC";
        ;;
    *)
        echo ""
        echo $"Usage: $0 firmware twi_address start_address {16|8|2|1} ";
        exit 2;
esac

echo "**************************************************************************"
echo "* Starting Timonel compilation with these parameters: "
echo "* --------------------------------------------------- "
echo "* Binary file: $ARG1.hex"
echo "* TWI address: $ARG2"
echo "* Flash position: $ARG3 <- use lower positions if make errors pop up below"
echo "* CPU clock speed: $ARG4 MHz <- $CLK_SOURCE (low fuse = $LOW_FUSE)"
echo "* Automatic clock tweaking: $ARG5"
echo "**************************************************************************"
echo ""

make clean_all
make clean_all TARGET=$ARG1
make all TARGET=$ARG1 TIMONEL_TWI_ADDR=$ARG2 TIMONEL_START=$ARG3 LOW_FUSE=$LOW_FUSE AUTO_CLK_TWEAK=$ARG5
cp *.hex releases
make clean_all TARGET=$ARG1

