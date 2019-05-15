#!/bin/sh
#############################################################
# MAKE_TIMONEL.sh                                           #
# ......................................................... #
# This script generates Timonel bootloader images that are  #
# flashable with an AVR programmer (e.g. USBasp).           #
#                                                           #
# ......................................................... #
# 2019-04-28 Gustavo Casanova                               #
# ......................................................... #
# From Linux: it runs directly.                             #
# From Windows/Mac: you need to install the Git Bash tool   #
# for your system (git-scm.com/downloads).                  #
#                                                           #
#############################################################

# Command line arguments
# ARG1: Timonel .hex filename. Default = timonel
# ARG2: Timonel TWI (I2C) address. Default = 8
# ARG3: Timonel start memory position. Default = 1B00 (hex)

ARG1=${1:-timonel}
ARG2=${2:-11}
ARG3=${3:-1A00}

echo "**************************************************************************"
echo "* Starting Timonel compilation with these parameters: "
echo "* --------------------------------------------------- "
echo "* Binary file: $ARG1.hex"
echo "* TWI address: $ARG2"
echo "* Flash position: $ARG3 <- use lower positions if make errors pop up below"
echo "**************************************************************************"

make clean_all
make clean_all TARGET=$ARG1
make all TARGET=$ARG1 TIMONEL_TWI_ADDR=$ARG2 TIMONEL_START=$ARG3
cp *.hex releases
make clean_all TARGET=$ARG1

