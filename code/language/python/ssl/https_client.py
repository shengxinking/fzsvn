#!/usr/bin/env python

from ssl import SSLSocket;
import sys

def _usage():
	print("https_cli.py <server IP> <server port> <certificate> <key>");

def _main():
	if ( sys.argc != 5):
		_usage();
		exit(-1);
	
	server_ip = sys.argv[1];
	server_port = sys.argv[2];
	certficate = sys.argv[3];
	privatekey = sys.argv[4];

	SSLSocket fd = SSLSocket.socket()
	wrap_socket(sock)

if __name__ == "__main__":
	_main(); 	
	

