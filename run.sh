#!/bin/bash
if [ -e "/usr/lib/libreadline.so" ]; then
	if [ ! -e "./minishell" ]; then
		gcc -w -s /usr/lib/libreadline.so /usr/lib/libhistory.so -o minishell rline.c main.c
		./minishell
	else
		./minishell
	fi
else
	echo "======================================";
	echo " Vous devez installer libreadline-dev ";
	echo "======================================";
	sudo apt-get install libreadline-dev

fi


