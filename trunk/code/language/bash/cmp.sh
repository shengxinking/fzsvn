#!/bin/bash

#if [ "$1" = "$2" ]; then
#    echo "$1 is equal $2"
#fi

#if [ "$1" != "$2" ]; then
#    echo "$1 is not equal $2"
#fi
#if [ "$1" > "$2" ]; then
#    echo "$1 large than $2"
#fi
#if [ "$1" < "$2" ]; then
#    echo "$1 less than $2"
#fi

if [ "-x$1" -eq "-x$2" ]; then
    echo "$1 is equal $2"
elif [ "-x$1" > "-x$2" ]; then
    echo "$1 large than $2"
else
    echo "$1 less than $2"
fi

