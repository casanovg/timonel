#!/bin/sh
# ----------------------
# 2019-06-06
# ----------------------
./MAKE_TIMONEL.sh timonel 11 1B80 1 false;
./flash-timonel-bootloader.sh timonel 1;
./fuse_read.sh;
