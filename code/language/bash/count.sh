#!/bin/bash

declare -i wordmax
declare -i wordcnt
declare word

function count
{
    for filename in "$*" ; do
	echo  ${filename}: 
        wordmax=`wc -l $filename | cut -d\  -f1`
        wordcnt=1
        while [ pwd ]
	do
	    if [ $wordcnt -gt $wordmax ]; then
		break
	    fi
	    word=`head -n $wordcnt $filename | tail -n 1`
	    echo -e -n "${word}:    \t"
	    echo  `grep -c $word $filename`
	    wordcnt=$(($wordcnt + 1))
        done
    done
}

count $*  | sort  -u -r
