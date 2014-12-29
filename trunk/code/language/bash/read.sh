#!/bin/bash

findterm()
{
	TERM=vt100
	lines="/dev/pts/1"
	while read devtype termtype; do
		if [ $devtype = $lines ]; then
			TERM=$termtype
			echo "TERM set to $TERM"
			break;
		fi
	done
}

findterm < terminfo
