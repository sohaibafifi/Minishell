#!/bin/bash
if [ -e "/usr/lib/libreadline.so" ]; then
	gcc -w  -s /usr/lib/libreadline.so /usr/lib/libhistory.so -o MiniShell rline.c main.c 
	./MiniShell
else
	echo "===================================="
        echo "Vous devez installer libreadline-dev"
	echo "===================================="
	sudo apt-get install libreadline-dev
fi

