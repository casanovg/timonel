#!/bin/sh
##############################################################
# MAKE_PAYLOAD.sh                                            #
# .......................................................... #
# This script generates Application Payloads to be included  #
# into the i2cmaster source code before compiling it.        #
# .......................................................... #
# 2018-09-09 Gustavo Casanova                                #
# .......................................................... #
# From Linux: it runs directly.                              #
# From Windows/Mac: you need to install the Git Bash tool    #
# (git-scm.com/downloads). Also add .exe extension to the    #
# parser command: "tml-hexparser.exe".                       #
#                                                            #
##############################################################

if [ "$1" != "" ]; then
    payload=$1
	echo
	# Parsing application to create payload header ...
	echo "Parsing" $1 "..."
	if [ `uname | grep Linux` ]; then
		./tml-hexparser $payload > ../timonel-i2cmlib/payloads/payload.h
		./tml-hexparser $payload > ./appl-payload/payload.h
	else
		./tml-hexparser.exe $payload > ../timonel-i2cmlib/include/payload.h
		./tml-hexparser.exe $payload > ./appl-payload/payload.h
	fi	
else
	echo
    echo "Usage: MAKE_PAYLOAD path_to_hexfile"
fi

