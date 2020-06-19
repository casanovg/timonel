#!/bin/sh
# ........................................................
# File: make-upload-default.sh
# Project: Timonel - TWI Bootloader
# ........................................................
# 2019-04-07
#

# Slave 1
./make-timonel.sh tml-t85-small timonel 29 1C40 1 false;

# Slave 2
#./make-timonel.sh tml-t85-std-norun-dump timonel 16 1B80 1 false;

./flash-timonel-bootloader.sh timonel 1;
./fuse-read.sh;
