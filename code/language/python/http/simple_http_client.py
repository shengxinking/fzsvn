#!/usr/bin/python

# filename: simple_http_client.py
# author: Forrest.zhang

import httplib
import sys

def usage():
	'''show usage message.'''
	print "http_simple_client.py <ip> <port> <POST|GET|PUT|HEAD> <url> <postfile>"


def run(addr, port, cmd, url, data):
	'''main function'''
	h = httplib.HTTPConnection(addr, port)
	h.request(cmd, url, data, {"Content-Type":"text/xml"});
	r = h.getresponse();
	print r.status
	h.close()


if len(sys.argv) != 6:
	usage()
	sys.exit()

addr = sys.argv[1]
port = sys.argv[2]
cmd = sys.argv[3]
url = sys.argv[4]
f = open(sys.argv[5])
data = f.read(20240000)

if __name__ == "__main__":
	run(addr, port, cmd, url, data)
	
