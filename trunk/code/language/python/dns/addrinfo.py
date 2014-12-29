#!/usr/bin/env python

import socket, sys

def usage():
    """ show usage information """
    print "addrinfo.py <host name>"


def getaddr(addr):
    """ get address infomation using socket.getaddrino """
    try:
        result = socket.getaddrinfo(addr, None, 0, socket.SOCK_STREAM)
    except socket.gaierror, e1:
        print "Raise error: %s" % e1
        return []

    except socket.error, e2:
        print "Raise error: %s" % e2
        return []

    return result


if __name__ == "__main__":
    if len(sys.argv) != 2:
        usage()
        sys.exit(0)

    res = getaddr(sys.argv[1])
    for i in range(len(res)):
        print "%-2d: %s" % (i, res[i])


