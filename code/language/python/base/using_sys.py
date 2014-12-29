#!/usr/bin/python

# filename: using_sys.py

import sys

print 'the command line arguments are:'
for i in sys.argv:
	print i

print '\n\nthe PYTHONPATH is', sys.path, '\n'
