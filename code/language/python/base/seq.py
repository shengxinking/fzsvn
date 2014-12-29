#!/usr/bin/python

# filename: seq.py

shoplist = ['apple', 'mango', 'carrot', 'banana']

print 'item 0 is', shoplist[0]
print 'item 1 is', shoplist[1]
print 'item 2 is', shoplist[2]
print 'item 3 is', shoplist[3]
print 'item -1 is', shoplist[-1]
print 'item -2 is', shoplist[-2]
print 'item -3 is', shoplist[-3]
print 'item -4 is', shoplist[-4]


print 'item 1 to 3 is', shoplist[3:0]
print 'item 1 to 4 is', shoplist[1:4]
print 'item 2 to end is', shoplist[2:]
print 'item 1 to -1 is', shoplist[1:-1]
print 'item start to end is', shoplist[:]

name = 'swaroop'
print 'characters 1 to 3 is', name[1:3]
print 'characters 2 to end is', name[2:]
print 'characters 1 to -1 is', name[1:-1]
print 'characters start to end is', name[:]
