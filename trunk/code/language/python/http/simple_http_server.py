#!/usr/bin/python

# filename: simple_http_server.py
# author: Forrest.zhang

import BaseHTTPServer
import sys

def usage():
	'''show usage message.'''
	print "http_simple_server.py <ip> <port>"

class SimpleHTTPRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
	server_version = "Forrest.zhang HTTP/1.0"

	def do_GET(self):
		print "client request a GET"
		self.send_error(404, "file not found")

	def do_POST(self):
		print "client %s:%d request a POST\n" %self.client_address
#		print "request headers is:"
#		print self.headers
		clen = self.headers.getheader("content-length")
		if clen:
			clen = int(clen)
		else:
			print "POST error: missing content-length"
			return
		body = self.rfile.read(clen)
#		print "\nPOST body is:"
#		print body
		self.send_response(200, "OK")

	def do_PUT(self):
		print "client request a PUT"
		self.send_error(404, "PUT not support")

	def do_HEAD(self):
		print "client request a HEAD"
		self.send_error(404, "HEAD not support")

def run(addr, port):
	svr = BaseHTTPServer.HTTPServer((addr, port), SimpleHTTPRequestHandler)
	svr.serve_forever()

if len(sys.argv) != 3:
	usage()
	sys.exit(0)

addr = sys.argv[1]
port = int(sys.argv[2])
run(addr, port)
