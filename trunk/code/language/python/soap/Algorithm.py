#!/usr/bin/python

import SOAPpy

wsdl = 'http://172.22.14.200:8080/axis/soap/Algorithm.jws?wsdl'
server = SOAPpy.WSDL.Proxy(wsdl)

def usage():
	""" show Algorithm usage """
	print "algorithm <add | sub | mul | div> <number 1> <number 2>"

def add(i, j):
	""" SOAP for Algorithm's Add method."""
	r = server.Add(float(i), float(j));
	print "Add %f %f result is: %f" %(float(i), float(j), r)

def sub(i, j):
	""" SOAP for Algorithm's Add method."""
	r = server.Sub(float(i), float(j));
	print "Sub %f %f result is: %f" %(float(i), float(j), r)

def mul(i, j):
	""" SOAP for Algorithm's Add method."""
	r = server.Mul(float(i), float(j));
	print "Mul %f %f result is: %f" %(float(i), float(j), r)

def div(i, j):
	""" SOAP for Algorithm's Add method."""
	r = server.Div(float(i), float(j));
	print "Div %f %f result is: %f" %(float(i), float(j), r)

if __name__ == "__main__":
	import sys
	if len(sys.argv) != 4:
		usage()
		sys.exit(0)

	if sys.argv[1] == "add":
		add(sys.argv[2], sys.argv[3]);

	elif sys.argv[1] == "sub":
		sub(sys.argv[2], sys.argv[3]);
	
	elif sys.argv[1] == "mul":
		mul(sys.argv[2], sys.argv[3]);

	elif sys.argv[1] == "div":
		div(sys.argv[2], sys.argv[3]);
	else:
		usage();
		sys.exit(1);
	


