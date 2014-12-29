#!/usr/bin/python

# filename: lambda.py

def make_repeater(n):
	return lambda s: s*n

twice = make_repeater(2)
third = make_repeater(3)

print twice('word')
print third(10)
