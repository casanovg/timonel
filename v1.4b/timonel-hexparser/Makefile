#
# Makefile for Timonel Intel Hex Parser
# =====================================
# (c) 2018 Gustavo Casanova
# gustavo.casanova@nicebots.com
#

CC=gcc

ifeq ($(shell uname), Linux)
	EXE_SUFFIX =
	OSFLAG = -D LINUX
else ifeq ($(shell uname), Darwin)
	EXE_SUFFIX =
	OSFLAG = -D MAC_OS
else ifeq ($(shell uname), OpenBSD)
	EXE_SUFFIX =
	OSFLAG = -D OPENBSD
else ifeq ($(shell uname), FreeBSD)
	EXE_SUFFIX =
	OSFLAG = -D OPENBSD
else
	EXE_SUFFIX = .exe
	OSFLAG = -D WIN
endif

LIBS =

PRDNAME = tml-hexparser

CFLAGS  = -O -g $(OSFLAG)

#FOLDER = $(lastword $(subst /,,$(CURDIR)))
FOLDER = timonel-hexparser

.PHONY:	clean

all: $(FOLDER)

$(FOLDER): $(addsuffix .o,)
	@echo
	@echo Building $(PRDNAME)$(EXE_SUFFIX) ...
	@echo ------------------------------
	$(CC) $(CFLAGS) -o $(PRDNAME)$(EXE_SUFFIX) $(PRDNAME).c $^ $(LIBS)
clean:
#	rm -f $(FOLDER)$(EXE_SUFFIX) *.o *.exe
	rm -f *.o

install: all
	cp $(PRDNAME) /usr/local/bin
