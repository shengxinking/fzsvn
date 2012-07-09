#!/bin/bash

#echo "1: install"

#echo "2: uninstall"

#echo "3: help"

PS3='please input number[1 2 3 4]:? '

IFS=":"

#echo "`dirname $0`"

selection="installed:uninstall:help:quit"

select name in $selection; do
	if [ -z $name ]; then
		echo "you input an error number"
	elif [ $name = "installed" ]; then
		echo "you select install"
	elif [ $name = "uninstall" ]; then
		echo "you select uninstall"
	elif [ $name = "help" ]; then
		echo "1: install"
		echo "2: uninstall"
		echo "3: help"
		echo "q: exit"
	elif [ $name = "quit" ]; then
		echo "bye"
		break
	fi
done

