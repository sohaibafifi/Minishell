export SHELL = bash
#DEBUG = no
PACKAGE = minishell
CC = gcc 

ifdef DEBUG
	CFLAGS =-s /usr/lib/libreadline.so /usr/lib/libhistory.so -o
else
	CFLAGS =-w -s /usr/lib/libreadline.so /usr/lib/libhistory.so -o
endif

default: all

all:  minishell

minishell:
	$(CC) $(CFLAGS) $(PACKAGE) rline.c main.c

clean:	
	rm -fr *.o *~  $(shell pwd)/$(PACKAGE)
	/bin/rm -f Makefile config.h config.status config.cache config.log

test:run

run:
	clear
	$(shell pwd)/$(PACKAGE)	
	
prerequisites:
	./build-prerequisites

.PHONY: default
.PHONY: all clean
