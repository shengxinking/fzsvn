#!/usr/bin/python

# filename: func_global.py

def func():

	global x

	print 'x is', x
	x = 2
	print 'changed global x to', x

x = 30
func()
print 'value of x is', x
