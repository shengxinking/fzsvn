#!/usr/bin/python

import sys, socket

def usage():
    """ show usage information """
    print "addrinfo.py <host name>"


def getname(addr):
    """ get address infomation using socket.getaddrino """

    try:
        result = socket.gethostbyaddr(addr)

    except socket.gaierror, e1:
        print "Raise error: %s" % e1
        return []

    except socket.error, e2:
        print "Raise error: %s" % e2
        return []

    except socket.herror, e:
        print "Raise error: %s" % e

    return result


if __name__ == "__main__":
    if len(sys.argv) != 2:
        usage()
        sys.exit(0)

    res = getname(sys.argv[1])
    print  res
