#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

REQUIRED_AUTOMAKE_VERSION=1.8

PKG_NAME="minishell"

autoconf && automake && ./configure && echo "all right !!! just type \"make\" to build $PKG_NAME "




