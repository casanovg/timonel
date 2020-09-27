#!/bin/sh
# ........................................................
# File: make-upload-default.sh
# Project: Timonel - TWI Bootloader
# ........................................................
# 2019-04-07
#

./make-timonel.sh tml-t85-std-dump timonel 32 1AC0 1 false;
#./make-timonel.sh tml-t85-std timonel 16 1C00 1 false;
./flash-timonel-bootloader.sh timonel 1;
./fuse-read.sh;
