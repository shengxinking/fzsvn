#!/usr/bin/python

# filename: try_except.py

import sys

try:
	s = raw_input('enter something --->')
except EOFError:
	print '\nwhy did you do an EOF on me?'
	sys.exit()
except:
	print '\nsome error/exception occured.'
	sys.exit()
else:
	print '\nno exception occured.'

print 'done'