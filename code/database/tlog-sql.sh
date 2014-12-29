#!/usr/bin/env bash

if [ $# != 1 ]; then
	echo "sqlfile.sh <sqlfile name>"
	exit 1
fi

echo -n  > $1

for ((i=1;i<100000;i++))
do
	echo "insert into test(fileid,offset,date,time,log_id,msg_id,type,subtype,pri,proto,service,policy,src,src_port,dst,dst_port,http_method,msg,week,month,hour,datetime) values($i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i,$i);" >> $1
done

