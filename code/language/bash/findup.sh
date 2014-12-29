#!/usr/bin/env bash

set -e
path="$1"
shift 1
while [[ "`readlink -f ${path}`" != "/" ]];
do
	file=$(find "$path" -maxdepth 1 -mindepth 1 -name "$@")
	if [ -n "$file" ]; then
		echo "$file"
		break
	fi
	path=${path}/..
done
