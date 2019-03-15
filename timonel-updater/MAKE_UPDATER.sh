#!/bin/sh
##############################################################
# MAKE_UPDATER.sh                                            #
# .......................................................... #
# This script generates Timonel bootloader updates that are  #
# flashable through Timonel itself.                          #
# .......................................................... #
# 2018-09-09 Gustavo Casanova                                #
# .......................................................... #
# From Linux: it runs directly.                              #
# From Windows/Mac: you need to install the Git Bash tool    #
# (git-scm.com/downloads).                                   #
#                                                            #
##############################################################

# 1) If there is no Timonel bootloader .hex, we make a new one ...
cd ../timonel-bootloader
./MAKE_TML.sh
# 2) We generate the Timonel+Updater .hex from the Timonel's .hex.
#    This file is placed in the "flashable-releases" folder and is
#    intended to be flashed with an AVR programmer (e.g. USBasp).
echo ""
echo "################################"
echo "#   >>> STARTING UPDATER <<<   #"
echo "################################"
echo ""
cd ../timonel-updater
ruby generate-data.rb ../timonel-bootloader/releases/tml-bootloader.hex
make clean
make
mv tml-updater.hex tmlupd-flashable
rm tml-payload.h
# 3) We generate the Timonel+Updater C header from the .hex file.
#    This file is placed in the "source-payloads" folder and is
#    intended to be included in the I2C master source code to be
#    flashed with a running Timonel through I2C.
ruby generate-data.rb ./tmlupd-flashable/tml-updater.hex
mv tml-payload.h tmlupd-payload
cp -f tmlupd-payload/tml-payload.h ../timonel-twim-ss/data/payloads/payload.h
cp -f tmlupd-payload/tml-payload.h ../timonel-twim-ms/data/payloads/payload.h
rm bootloader_data.c
make clean
