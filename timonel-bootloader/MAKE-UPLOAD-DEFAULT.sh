#!/bin/sh
./MAKE_TIMONEL.sh timonel 33 1C00 1 false; ./flash-timonel-bootloader.sh timonel 1; ./fuse_read.sh
