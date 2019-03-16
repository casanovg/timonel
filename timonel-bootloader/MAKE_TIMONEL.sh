#!/bin/sh
#############################################################
# MAKE_TML.sh                                               #
# ......................................................... #
# This script generates Timonel bootloader images that are  #
# flashable with an AVR programmer (e.g. USBasp).           #
#                                                           #
# ......................................................... #
# 2019-03-16 Gustavo Casanova                               #
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
ARG2=${2:-8}
ARG3=${3:-1B00}

make clean_all
make clean_all TARGET=$ARG1
make all TARGET=$ARG1 TIMONEL_TWI_ADDR=$ARG2 TIMONEL_START=$ARG3
cp *.hex releases
make clean_all TARGET=$ARG1

#echo ""
#echo "##########################"
#echo "#   >>> TML PASS 1 <<<   #"
#echo "##########################"
#echo ""
#cp tml-t85-std/make-pass-1.mak Makefile
#make clean_all
#make all
#make squeaky_clean
#echo ""
#echo "##########################"
#echo "#   >>> TML PASS 2 <<<   #"
#echo "##########################"
#cp tml-t85-std/make-pass-2.mak Makefile
#make clean_all
#make all
#cp *.hex releases
#make clean_all
#cp tml-t85-std/make-pass-1.mak Makefile