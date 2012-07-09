#!/bin/bash


if [ $1 -eq $2 ]; then
    echo "$1 equal $2"
fi
if [ $1 -ne $2 ]; then
    echo "$1 not equal $2"
fi
if [ $1 -lt $2 ]; then
    echo "$1 less than $2"
fi
if [ $1 -le $2 ]; then
    echo "$1 less than or equal $2"
fi
if [ $1 -gt $2 ]; then
    echo "$1 large than $2"
fi
if [ $1 -ge $2 ]; then
    echo "$1 large than or equal $2"
fi
