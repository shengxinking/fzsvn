#!/usr/bin/env python

# filename: apihelper.py

def info(obj, spacing = 10, collapse = 1):
	'''Print methods and doc strings.

	Takes module, class, list, dictionary, or string.'''
	methList = [m for m in dir(obj) if callable(getattr(obj, m))]
	proc_func = collapse and (lambda s: " ".join(s.split())) or (lambda s: s)
	print "\n".join(["%s %s" % \
		(m.ljust(spacing), \
		proc_func(str(getattr(obj, m).__doc__))) \
		for m in methList])

if __name__ == "__main__":
	print info.__doc__
