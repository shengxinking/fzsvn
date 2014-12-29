#!/usr/bin/python

# filename: using_cpickle

import cPickle as p

shoplist_file = 'shoplist.data'

shoplist = ['apple', 'mango', 'carrot']

f = file(shoplist_file, 'w')
p.dump(shoplist, f)
f.close

f = file(shoplist_file)
storedlist = p.load(f)

print storedlist