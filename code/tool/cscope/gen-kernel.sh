#!/usr/bin/env bash

find $PWD -path "$PWD/arch/*" ! -path "$PWD/arch/i386*" -prune -o \
-path "$PWD/include/asm-*" ! -path "$PWD/include/asm-i386*" -prune -o \
-path "$PWD/tmp*" -prune -o \
-path "$PWD/Documentation*" -prune -o \
-path "$PWD/scripts*" -prune -o \
-path "$PWD/drivers*" -prune -o \
-name "*.[chxsS]" -print > $PWD/cscope.files

cscope -b -q -k

