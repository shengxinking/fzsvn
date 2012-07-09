#!/usr/bin/python

while True:
	s = raw_input('enter something: ')
	if s == 'quit':
		break
	if len(s) < 3:
		continue
	print 'length of the string is', len(s)

print 'done'
