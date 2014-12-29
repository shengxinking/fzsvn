#!/usr/bin/env	bash


declare -i success=0
declare -i failed=0
declare -i count=1

#
#	Show script usage
#
function usage
{
    echo "curlloop: using curl get pages and loop some times"
    echo "usage: curlloop.sh <loop count> <url>"
}


# check the arguments
if [ $# != "2" ]; then
    usage
    exit 0
fi

count=$1
url=$2
declare -i i=0

echo "--------- begin get $url $count times -------"

while [ $i -lt $count ]; do
    curl -k $url
    if [ $? == "0" ]; then
	let success=$success+1
    else
	let failed=$failed+1
    fi
    let i=$i+1
done

echo "-------------------- end --------------------"

echo "get url: $url $count times, success $success failed $failed"
