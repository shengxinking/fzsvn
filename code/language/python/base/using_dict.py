#!/usr/bin/python

# filename: using_dict.py

ab = { 'swaroop' : 'swaroopch@byteofpython.info',
	'larry'  : 'larry@wall.org',
	'matsumoto' : 'matz@ruby-lang.org',
	'spammer' : 'spammer@hotmail.com' }

print "swaroop's address is %s" % ab['swaroop']

ab['guido'] = 'guido@python.org'

del ab['spammer']

print '\nthere are %d contacts in the address-book\n' % len(ab)

for name, address in ab.items():
	print 'contact %s at %s' % (name, address)

if 'guido' in ab:
	print 'guido\'s address is %s' %ab['guido']
