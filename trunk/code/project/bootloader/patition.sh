#!/usr/bin/env bash

#	Bash script file
#	Write by Forrest.zhang

if -z $1; then
    echo "patition <device file or loop file>"
    exit -1
fi

echo "Create a partion table on $1"

echo "1000 80000 83 *" > $PWD/.tmpsfdisk
echo "81000 80000 83" >> $PWD/.tmpsfdisk

sudo sfdisk -q -f -uS $1 < $PWD/.tmpsfdisk

rm -f $PWD/.tmpsfdisk

echo "Patition success, using list_mbr to see patition table\n"

