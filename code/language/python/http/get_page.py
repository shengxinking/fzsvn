#!/usr/bin/python

# filename: get_page.py

import httplib
import sys

def get_page(url):
	conn = httplib.HTTPConnection('172.22.14.200')
	conn.request("GET", url)
	response = conn.getresponse()
	html = response.read()
	conn.close()
	return html

page = get_page(sys.argv[1])
print page


