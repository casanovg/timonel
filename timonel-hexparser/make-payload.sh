#!/bin/bash
##############################################################
# MAKE_PAYLOAD.sh                                            #
# .......................................................... #
# This script generates Application Payloads to be included  #
# into the i2cmaster source code before compiling it.        #
# .......................................................... #
# 2018-12-04 Gustavo Casanova                                #
# .......................................................... #
# From Linux: it runs directly.                              #
# From Windows/Mac: you need to install the Git Bash tool    #
# (git-scm.com/downloads). Also add .exe extension to the    #
# parser command: "tml-hexparser.exe".                       #
#                                                            #
##############################################################

if [ "$1" != "" ]; then
    payload=$1;
	if [ $payload == "-h" ] || [ $payload == "--h" ] || [ $payload == "-help" ] || [ $payload == "--help" ] || [ $payload == "-?"  ]; then
        echo "";
        echo "Usage: MAKE_PAYLOAD path_to_hexfile";
        exit 1;
    fi
    if [ ! -f $payload ]; then
        echo "";
        echo "Binary file not found!";
        exit 2;
    fi
	# Parsing application to create payload header ...
	echo "Parsing" $1 "...";
	if [ `uname | grep Linux` ]; then
		#.pio/build/native/tml-hexparser $payload > ../timonel-twim-ss/data/payloads/payload.h;
		.pio/build/native/tml-hexparser $payload > ./appl-payload/payload.h;
	else
		#.pio/build/native/tml-hexparser.exe $payload > ../timonel-twim-ss/data/payloads/payload.h;
		.pio/build/native/tml-hexparser.exe $payload > ./appl-payload/payload.h;
	fi	
else
	echo "";
    echo "Usage: MAKE_PAYLOAD path_to_hexfile";
    exit 1;
fi

