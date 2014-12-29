#!/bin/bash

declare -i totalsize
let totalsize=0

for dir in ${*:-.}; do
	if [ -e $dir ]; then
		result=$(du -s $dir | cut -f 1)
		let total=$result*1024

		echo -n "Total for $dir = $total bytes"

		if [ $total -ge 1048576 ]; then
			echo " ($((total/1048576)) Mb)"
		elif [ $total -ge 1024 ]; then
			echo " ($((total/1024)) Kb)"
		fi
		let totalsize=totalsize+total
	fi
done

echo -n "Total size = $totalsize bytes"

if [ $totalsize -ge 1048576 ]; then
	echo " ($((totalsize/1048576)) Mb)"
elif [ $totalsize -ge 1024 ]; then
	echo " ($((totalsize/1024)) Kb)"
fi
	
