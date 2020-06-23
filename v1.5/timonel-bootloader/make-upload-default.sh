#!/bin/sh
# ........................................................
# File: make-upload-default.sh
# Project: Timonel - TWI Bootloader
# ........................................................
# 2019-04-07
#

# Slave 1
#./make-timonel.sh tml-t85-small timonel 33 1C80 1 false;
#./make-timonel.sh tml-t85-small timonel 31 1B80 8 false;

# Slave 2
#./make-timonel.sh tml-t85-std-norun-dump timonel 16 1B80 1 false;

# Slave 3
./make-timonel.sh tml-t85-small-dump timonel 35 1B40 1 false

./flash-timonel-bootloader.sh timonel 1;
./fuse-read.sh;
