#!/bin/sh
# ........................................................
# File: make-upload-default.sh
# Project: Timonel - TWI Bootloader
# ........................................................
# 2019-04-07
#

./make-timonel.sh tml-t85-std timonel 11 1C00 1 false;
./flash-timonel-bootloader.sh timonel 1;
./fuse-read.sh;
