#!/usr/bin/python

try:
	fp = open("haha.txt", "r")
except IOError:
	print "I catched IOError"

print "I'm here"

