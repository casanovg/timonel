#!/bin/sh
#############################################################
# MAKE_TML.sh                                               #
# ......................................................... #
# This script generates Timonel bootloader images that are  #
# flashable with an AVR programmer (e.g. USBasp).           #
#                                                           #
# It's a bit messy, but it gets smaller images by making it #
# with Pass-1.mak to generate the nb-usitwisl-if.o file,    #
# then making it again Pass-2.mak to place it in an upper   #
# flash location. It's needless to say that writing         #
# Makefiles is not my strength, so any help to improve      #  
# this is very welcome.                                     #
#                                                           #
# ......................................................... #
# 2018-09-16 Gustavo Casanova                               #
# ......................................................... #
# From Linux: it runs directly.                             #
# From Windows/Mac: you need to install the Git Bash tool   #
# for your system (git-scm.com/downloads).                  #
#                                                           #
#############################################################
echo ""
echo "##########################"
echo "#   >>> TML PASS 1 <<<   #"
echo "##########################"
echo ""
cp tml-t85-std/make-pass-1.mak Makefile
make clean_all
make all
make squeaky_clean
echo ""
echo "##########################"
echo "#   >>> TML PASS 2 <<<   #"
echo "##########################"
cp tml-t85-std/make-pass-2.mak Makefile
make clean_all
make all
cp *.hex releases
make clean_all
cp tml-t85-std/make-pass-1.mak Makefile