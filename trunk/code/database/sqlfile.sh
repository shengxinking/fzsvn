#!/usr/bin/env bash

if [ $# != 1 ]; then
	echo "sqlfile.sh <sqlfile name>"
	exit 1
fi

echo -n  > $1

for ((i=1;i<100000;i++))
do
	echo "insert into test(fileid,offset,date,time) values($i,$i,$i,$i);" >> $1
done

