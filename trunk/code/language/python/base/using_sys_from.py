#!/usr/bin/python

# filename: using_sys.py
from sys import *

print 'the command line arguments are:'
for i in argv:
	print i


print '\n\nthe PYTHONPATH is', path, '\n'
