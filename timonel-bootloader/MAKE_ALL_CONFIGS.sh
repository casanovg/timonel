#!/bin/sh
#############################################################
# MAKE_ALL_CONFIGS.sh                                       #
# ......................................................... #
# This script generates all Timonel bootloader options that #
# are under the "configs" directory. The generated images   #
# can be flashed with an AVR programmer (e.g. USBasp).      #
# ......................................................... #
# 2019-06-06 Gustavo Casanova                               #
# ......................................................... #
# From Linux: it should run directly.                       #
# From Windows/Mac: you need to install the Git Bash tool   #
# for your system (git-scm.com/downloads).                  #
#                                                           #
#############################################################

BIN_DIR="releases";
CFG_DIR="configs";
CONFIGS=`ls -l $CFG_DIR | awk '{print $9}'`;

for TML_CFG in $CONFIGS; do
    echo "Making:" ${TML_CFG};
    make all CONFIG=${TML_CFG} TARGET=${TML_CFG};
    mv ${TML_CFG}.hex ./$BIN_DIR;
    make clean_all CONFIG=${TML_CFG} TARGET=${TML_CFG};
    echo "...............................";
done
