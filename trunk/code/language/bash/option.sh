#!/bin/bash

while getopts ":hf:o:" opts; do
	case $opts in
		h )  echo "options.sh -h:            help info"
		     echo "options.sh -o <options> : set options"
		     echo "options.sh -f <filename>: set input file";;
		f )  echo "options.sh -f $OPTARG";;
		o )  echo "options.sh -o $OPTARG";;
		? )  echo "invalid option";;
	esac
done
