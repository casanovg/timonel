#!/bin/sh
##############################################################
# MAKE_UPDATER.sh                                            #
# .......................................................... #
# This script generates Timonel bootloader updates that are  #
# flashable through Timonel itself.                          #
# .......................................................... #
# 2019-01-16 Gustavo Casanova                                #
# .......................................................... #
# From Linux: it runs directly.                              #
# From Windows/Mac: you need to install the Git Bash tool    #
# (git-scm.com/downloads).                                   #
#                                                            #
##############################################################

# Command line arguments
# ----------------------
# ARG1: Timonel configuration file. Default [ ARG1=${1:-tml-t85-std} ]
# ARG2: Timonel .hex filename. Default [ ARG2=${2:-timonel} ]
# ARG3: Timonel TWI (I2C) address. Default [ ARG3=${3:-13} ]
# ARG4: Timonel start memory position. Default [ ARG4=${4:-1B80} ]
# ARG5: LOW_FUSE clock adjustments. Valid options = 16, 8, 2, 1. Default [ ARG5=${5:-1} ]
# ARG6: Automatic clock tweaking. When is enabled, it overrides LOW_FUSE value. Default [ ARG6=${6:-false} ]

BIN_DIR="releases";
CFG_DIR="configs";
CFG_DFT="tml-t85-std"
TML_CFG="tml-config.mak";
TML_DIR="../timonel-bootloader"
OWN_DIR=`pwd`;
HEX_SFX=".hex";
MAK_OPT="";

ARG1=${1:-$CFG_DFT};
ARG2=${2:-timonel};
ARG3=$3;    # ARG3=${3:-13};
ARG4=$4;    # ARG4=${4:-1B80};
ARG5=$5;    # ARG5=${5:-1};
ARG6=$6;    # ARG6=${6:-false};

function print_help {
    echo "";
    echo "Usage: $0 [(<CONFIG> <FW_NAME> <TWI_ADDR> <START_ADDR>";
    echo "                          (<16>|<8>|<2>|<1>) (<false>|<true>))]";
    echo "       $0 [(-h | --help)]";
    echo "";
    echo "Generates Timonel a custom payload to compile with the TWI master firmware.";
    echo "";
    echo "Arguments:";
    echo "  CONFIG      Timonel configuration option to use. (Def=tml-t85-std)."
    echo "  FW_NAME     Name of the .hex binary file to produce. (Def=timonel)";
    echo "  TWI_ADDR    TWI (I2C) address to assign to the device. Range: 8-35 (Def=11).";
    echo "  START_ADDR  Bootloader start address in the device memory. Range: 0-1C00.";
    echo "  CLK_SPEED   Device speed settings (in MHz). Values: 1, 2, 8 or 16 (Def=1).";
    echo "  AUTO_TWEAK  Defines if the device speed adjustments will be made at";
    echo "              run time. Valid options: false-true (Def=false).";    
    echo "";
    echo "Options:";
    echo "  -h --help   Prints this help.";
    echo "";
    echo "Examples:";
    echo "  $" $0;
    echo "  Generates a payload file based on \"tml-t85-std\" defaults->FW_NAME=timonel,";
    echo "  TWI_ADDR=11, START_ADDR=0x1B80, CLK_SPEED=1 (MHz), AUTO_TWEAK=false.";
    echo "";
    echo "  $ $0 tml-t85-full";
    echo "  Generates a payload file based on \"tml-t85-full\" config.";
    echo "";    
    echo "  $ $0 tml-t85-small new-test 17 1B00 8 false";
    echo "  Generates a payload file based on \"tml-t85-small\" config,";
    echo "  assigning TWI address 17 to the device, setting 0x1B00 device memory position";
    echo "  as bootloader start, setting the device low fuse to operate at 8 MHz";
    echo "  and disabling automatic clock tweaking.";
}

case ${ARG1} in
    -h|-help|--h|--help)
        print_help;
        exit;
        ;;
    *)
        if [ ! -f "${TML_DIR}/${CFG_DIR}/${ARG1}/${TML_CFG}" ]; then
            echo "";
            echo "Config \"${ARG1}\" not found in \"${CFG_DIR}\" directory!";
            exit 1;
        fi
        ;;
esac

if [ ! -z "${ARG1}" ]; then
    MAK_OPT+="${ARG1}";
fi
if [ ! -z "${ARG2}" ]; then
    MAK_OPT+=" ${ARG2}";
fi
if [ ! -z "${ARG3}" ]; then
    MAK_OPT+=" ${ARG3}";
fi
if [ ! -z "${ARG4}" ]; then
    MAK_OPT+=" ${ARG4}";
fi
if [ ! -z "${ARG5}" ]; then
    MAK_OPT+=" ${ARG5}";
fi
if [ ! -z "${ARG6}" ]; then
    MAK_OPT+=" ${ARG6}";
fi

echo
echo "STRING: ${MAK_OPT}";

# 1) Make a new Timonel bootloader .hex file ...
cd ${TML_DIR}
./make-timonel.sh ${MAK_OPT};

# 2) Generate the Timonel+Updater .hex from the Timonel's .hex.
#    This file is placed in the "flashable-releases" folder and is
#    intended to be flashed with an AVR programmer (e.g. USBasp).
echo ""
echo "################################"
echo "#   >>> STARTING UPDATER <<<   #"
echo "################################"
echo ""
cd ../timonel-updater
ruby generate-data.rb ${TML_DIR}/${BIN_DIR}/${ARG2}.hex
make clean
make
mv tml-updater.hex tmlupd-flashable
rm tml-payload.h
# 3) Generate the Timonel+Updater C header from the .hex file.
#    This file is placed in the "source-payloads" folder and is
#    intended to be included in the I2C master source code to be
#    flashed with a running Timonel through I2C.
ruby generate-data.rb ./tmlupd-flashable/tml-updater.hex
mv tml-payload.h tmlupd-payload
cp -f tmlupd-payload/tml-payload.h ../timonel-twim-ss/data/payloads/payload.h
rm bootloader_data.c
make clean
