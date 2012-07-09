#!/bin/bash

arg1=$1
arg2=$2

if [ "${arg1}" < "${arg2}" ]; then
    echo "$arg1 less than $arg2"
else
    echo "$arg2 less than $arg1"
fi
