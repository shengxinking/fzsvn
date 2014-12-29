#!/usr/bin/python

# filename: func_default.py

def say(message = "Hello", times = 1):
	print message * times

say()
say('hello')
say('world', 5)
