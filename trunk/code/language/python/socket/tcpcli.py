#!/usr/bin/env python2.7

# filename: tcpcli.py

import socket
import sys

def usage():
	""" show usage infomation!"""
	print "tcpcli.py <ip> <port>"

def tcpcli(addr, port):
	"""connect to server(addr, port) send data, then close

	This is a test program for XML test"""

	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
	try :
		s.connect((addr, port))
		s.sendall("Hellow server\n");
		print "send bytes to %s:%d" %(addr, port)
		msg = s.recv(100)
		print msg
		
        except socket.error, e:
		(errno, errmsg) = e 
		print "error raised: %s" %e
		if errno == 113:
			print "we have c macro EHOSTUNREACH"
		print "type %d message %s" %(errno, errmsg)

	s.close()


if __name__ == "__main__":

	if len(sys.argv) != 3:
		usage()
		sys.exit(0)
		
	addr = sys.argv[1]
	port = int(sys.argv[2])
	tcpcli(addr, port);

