#!/usr/bin/python

# filename: finally.py

import time
import sys

try:
	f = file('poem.txt')
	while True:
		line = f.readline()
		if len(line) == 0:
			break;
		time.sleep(2)
		print line,

finally:
	f.close()
	print 'cleaning up...'