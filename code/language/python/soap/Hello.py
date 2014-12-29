#!/usr/bin/python

from SOAPpy import WSDL

WSDLFILE = 'http://172.22.14.200:8080/axis/soap/Hello.jws?wsdl'

_server = WSDL.Proxy(WSDLFILE)
def hello(q):
	""" commit a SOAP message """
	results = _server.hello(q)
	print results
	return 0

def usage():
	""" show usage message."""
	print "wsdl.py <user name>"


if __name__ == "__main__":
	import sys
	if len(sys.argv) != 2:
		usage()
		sys.exit(0)
	hello(sys.argv[1])

