#!/bin/sh
# ----------------------
# 2019-04-07
# ----------------------
./make-timonel.sh timonel 11 1B80 1 false;
./flash-timonel-bootloader.sh timonel 1;
./fuse-read.sh;