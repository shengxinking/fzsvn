#!/usr/bin/python

# filename: connect_string.py

def connect_string(params):
	"""build a connection string from a dictionary of parameters.

	return string."""
	return ";".join(["%s=%s" % (k, v) for k, v in params.items()])

if __name__ == "__main__":
	my_params = {"server":"mpilgrim", \
		     "database":"master", \
		     "uid":"sa",
		     "pwd":"secret" }

	print connect_string(my_params)
else:
	print "in module mode"
