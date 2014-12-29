#!/usr/bin/python

# filename: str_methods.py

name = 'swaroop'

if name.startswith('swq'):
	print 'yes, the string starts with "swa"'

if 'a' in name:
	print 'yes, it contains the string "a"'

if name.find('war') != -1:
	print 'yes, it contains the string "war"'

delimiter = '_*_'

mylist = ['brazil', 'russia', 'india', 'china']
print delimiter.join(mylist)


