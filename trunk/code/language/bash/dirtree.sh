#/bin/bash

pf1="|"
pf2="-"
pf3=" "

declare -i tab=4
declare -i level=0

printn()
{
    local i
    let i=$tab-1
    
    while [ $i -ne "0" ]; do
	echo -n "$1"
	let i=$i-1
    done
}

printnw()
{
    local i
    let i=$tab-1
    
    while [ $i -ne "0" ]; do
	echo -n "$pf3"
	let i=$i-1
    done
}

printpf1()
{
    local i;
    let i=$1;
    while [ $i -ne "0" ]; do
	echo -n "$pf1";
#	printnw
	printn "$pf3"
	let i=$i-1
    done
}

printpf2()
{
    echo -n "$pf1";
    printn $pf2
}

listdir()
{
    local   i;
    let i=$2;
    list=$(command ls $1);
    
    for file in $list ; do
	file=$1/$file
	if [ -d "$file" ]; then
	    printpf1 $i
	    printpf2;
	    echo "${file##*/}"
	    let i=$i+1
	    listdir "${file}" $i
#	    printpf1 $i
	    let i=$i-1
	    continue
	fi
	if [ -f $file ]; then
	    printpf1 $i
	    printpf2;
	    echo "${file##*/}"
	    continue
	fi
   done
#   echo ""
}

for filename in "$@"; do
	if [ -e $filename ]; then
	    echo "$filename"
	    if [ -d $filename ]; then
		listdir $filename $level
	    fi
	else
	    echo "$filename not exist"
	fi

	echo -e "\n"
done

