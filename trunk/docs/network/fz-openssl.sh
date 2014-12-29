#!/usr/bin/env	bash


function _certs_usage()
{
	echo -e "fz-openssl.sh <options>\n"
	echo -e "\tinit\tinit the openssl ca environment"
	echo -e "\tclean\tclean the openssl ca environment"
	echo -e "\thelp\tshow help message\n"
	exit 0
}


function _certs_init()
{
	echo -e "create directory for certs ..."
	if [ -d ./fzcerts ]; then
		echo "the fzcerts directory is exist"
		exit 1
	fi

	mkdir ./fzcerts
	mkdir ./fzcerts/newcerts

	echo -e "create files for ca ..."
	touch ./fzcerts/index.txt
	touch ./fzcerts/index.txt.attr
	touch ./fzcerts/rand
	echo "01" > ./fzcerts/serial

	echo -e "create root certificate ..."
	#openssl genrsa -out ./fzcerts/rootkey.pem 1024
	#openssl dsaparam -genkey -out ./fzcerts/rootkey.pem 1024
	openssl ecparam -genkey -out ./fzcerts/rootkey.pem -name prime256v1
	openssl req -x509 -new -key ./fzcerts/rootkey.pem -days 3650 -out ./fzcerts/rootca.pem
	
	echo -e "init ca environment finished"
}


function _certs_clean()
{
	echo -e "clean ca environment..."

	if [ -d ./fzcerts ]; then
		rm -rf ./fzcerts
	fi
}


if [ $# != 1 ]; then
	_certs_usage
fi

if [ $1 == "init" ]; then
	_certs_init
elif [ $1 == "clean" ]; then
	_certs_clean
elif [ $1 == "help" ]; then
	_certs_usage
else
	_certs_usage
fi

