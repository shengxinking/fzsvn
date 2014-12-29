#!/usr/bin/env bash

if [ $# != 1 ]; then
	echo "datafile.sh <datafile name>"
	exit 1
fi

echo -n  > $1

for ((i=1;i<200001;i++))
do
	echo "$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i" >> $1
done

