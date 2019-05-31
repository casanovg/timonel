#!/bin/sh
# ........................................................
# File: make-upload-default.sh
# Project: Timonel - TWI Bootloader
# ........................................................
# 2019-04-07
#

./make-timonel.sh tml-t85-small timonel 13 1B80 1 false;
./flash-timonel-bootloader.sh timonel 1;
./fuse-read.sh;
