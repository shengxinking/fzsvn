#!/usr/bin/python

# filename: connstr.py

def connstr(params):
	"""Build a connection string from a dictionary of parameters

	Return string."""
	return ';'.join(["%s=%s" % (k, v) for k, v in params.items()])

if __name__ == "__main__":
	myparams = {"server":"mpilgrim", \
		    "database":"master", \
		    "uid":"sa", \
		    "pwd":"secret" }
	print connstr(myparams)

