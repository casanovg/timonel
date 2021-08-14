#!/bin/bash
#############################################################
# MAKE_TIMONEL.sh                                           #
# ......................................................... #
# This script generates a Timonel bootloader image that is  #
# flashable with an AVR programmer (e.g. USBasp).           #
#                                                           #
# ......................................................... #
# 2019-04-28 Gustavo Casanova                               #
# ......................................................... #
# From Linux: it should run directly.                       #
# From Windows/Mac: you need to install the Git Bash tool   #
# for your system (git-scm.com/downloads).                  #
#                                                           #
#############################################################

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
HEX_SFX=".hex";
MAK_OPT="";

ARG1=${1:-$CFG_DFT};
ARG2=$2;    # ARG2=${2:-timonel};
ARG3=$3;    # ARG3=${3:-11};
ARG4=$4;    # ARG4=${4:-1C00};
ARG5=$5;    # ARG5=$5;
ARG6=$6;    # ARG6=$6;
# ARG5=$5;    # ARG5=${5:-1};
# ARG6=$6;    # ARG6=${6:-false};

print_help() {
    echo "";
    echo "Usage: $0 [(<CONFIG> <FW_NAME> <TWI_ADDR> <START_ADDR>";
    echo "                          (<16>|<8>|<2>|<1>) (<false>|<true>))]";
    echo "       $0 [(-h | --help)]";
    echo "       $0 [(-a | --all)]";    
    echo "";
    echo "Generates Timonel custom binary images to flash in an AVR microcontroller.";
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
    echo "  -a --all    Generates all Timonel configurations.";    
    echo "";
    echo "Examples:";
    echo "";
    echo "  $" $0;
    echo "";
    echo "  Generates a binary file based on \"tml-t85-std\" defaults->FW_NAME=timonel,";
    echo "  TWI_ADDR=11, START_ADDR=0x1B80, CLK_SPEED=1 (MHz), AUTO_TWEAK=false.";
    echo "";
    echo "  $ $0 tml-t85-full";
    echo "";
    echo "  Generates a \"timonel.hex\" binary file based on \"tml-t85-full\" config.";
    echo "";    
    echo "  $ $0 tml-t85-small new-test 17 1B00 8 false";
    echo "";
    echo "  Generates a \"new-test.hex\" binary file based on \"tml-t85-small\" config,";
    echo "  assigning TWI address 17 to the device, setting 0x1B00 device memory position";
    echo "  as bootloader start, setting the device low fuse to operate at 8 MHz";
    echo "  and disabling automatic clock tweaking.";
}

case ${ARG1} in
    -h|-help|--h|--help)
        print_help;
        exit;
        ;;
    -a|-all|--a|--all)
        echo "";
        echo "******************************************************"
        echo "* Starting compilation of all Timonel configurations *"
        echo "******************************************************"
        for TML_CFG in `ls -1 configs | awk -F'.' '{print $1}'`; do
            echo "";
            echo "MAKING ->" ${TML_CFG};
            echo "";
            make all CONFIG=${TML_CFG} TARGET=${TML_CFG};
            mv ${TML_CFG}${HEX_SFX} ./${BIN_DIR}/;
            make clean_all CONFIG=${TML_CFG} TARGET=${TML_CFG};
        done
        exit;
        ;;
    *)
        if [ ! -f "./${CFG_DIR}/${ARG1}.mak" ]; then
            echo "";
            echo "Configuration \"${ARG1}\" not found in \"${CFG_DIR}\" directory!";
            exit 1;
        fi
        ;;
esac

if [ ! -z "${ARG5}" ]; then
    case ${ARG5} in
        16)
            # echo "";
            # echo "Low fuse set for sixteen MHz internal clock source.";
            LOW_FUSE=0xE1;
            CLK_SOURCE="HF PLL";
            ;;
        8)
            # echo "";
            # echo "Low fuse set for eight MHz internal clock source.";
            LOW_FUSE=0xE2;
            CLK_SOURCE="RC OSC";
            ;;
        2)
            # echo "";
            # echo "Low fuse set for two MHz internal clock source.";
            LOW_FUSE=0x61;
            CLK_SOURCE="HF PLL";
            ;;
        1)
            # echo "";
            # echo "Low fuse set for one MHz internal clock source.";
            LOW_FUSE=0x62;
            CLK_SOURCE="RC OSC";
            ;;
        *)
            print_help;
            exit 2;
            ;;
    esac
fi

echo "";
echo "**************************************************************************";
echo "* Starting Timonel compilation with these parameters: ";
echo "* --------------------------------------------------- ";
echo "*    Configuration: $ARG1";

if [ ! -z "${ARG1}" ]; then
    MAK_OPT+="CONFIG=${ARG1}";
fi
if [ ! -z "${ARG2}" ]; then
    MAK_OPT+=" TARGET=${ARG2}";
    echo "*      Binary file: ${ARG2}${HEX_SFX}";
fi
if [ ! -z "${ARG3}" ]; then
    MAK_OPT+=" TIMONEL_TWI_ADDR=${ARG3}";
    echo "*      TWI address: ${ARG3}";
fi
if [ ! -z "${ARG4}" ]; then
    MAK_OPT+=" TIMONEL_START=${ARG4}";
    echo "*   Flash position: ${ARG4} <- In case of errors, use lower positions!";
fi
if [ ! -z "${ARG5}" ]; then
    MAK_OPT+=" LOW_FUSE=${LOW_FUSE}";
    echo "*  CPU clock speed: ${ARG5} MHz <- ${CLK_SOURCE} (low fuse = ${LOW_FUSE})";
fi
if [ ! -z "${ARG6}" ]; then
    MAK_OPT+=" AUTO_CLK_TWEAK=${ARG6}";
    echo "* Auto clock tweak: ${ARG6}";
fi

echo "**************************************************************************";
echo "";

make clean_all;
make clean_all ${MAK_OPT};
echo "RUNNING: make all ${MAK_OPT}";
# echo "";
make all ${MAK_OPT};
cp *${HEX_SFX} ${BIN_DIR};
make clean_all ${MAK_OPT};
