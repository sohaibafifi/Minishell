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

all: prerequisites minishell

minishell:
	$(CC) $(CFLAGS) $(PACKAGE) rline.c main.c

clean:	rm -r $(shell pwd)/$(PACKAGE)

test:run

run:
	clear
	$(shell pwd)/$(PACKAGE)	
	
prerequisites:
	./build-prerequisites

.PHONY: default
.PHONY: all clean
