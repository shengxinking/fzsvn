#!/usr/bin/python

# filename: raise.py

import sys

class short_exception(Exception):
	'''a use-defined exception class.'''
	def __init__(self, length, atleast):
		Exception.__init__(self)
		self.length = length
		self.atleast = atleast

try:
	s = raw_input('enter something --->')
	if len(s) < 3:
		raise short_exception(len(s), 3)

except EOFError:
	print '\nwhy did you do an EOF on me?'
	sys.exit(1)
except short_exception, x:
	print '\nshort_exception: the input was length %d, \
	       was expecting at least %d' %(x.length, x.atleast)
	sys.exit(1)
else:
	print '\nno exception was raised.'

print '\ndone'