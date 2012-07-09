#!/bin/sh

/usr/bin/rdate -s clock.psu.edu
/sbin/hwclock --systohc --localtime
