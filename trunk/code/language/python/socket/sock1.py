#!/usr/bin/python

import sys
import os
import socket

if __name__ != "__main__":
    sys.exit(0)

hostname = socket.gethostname()
print "host name is " + hostname

hostname = "www.sina.com.cn"
addr = socket.gethostbyname(hostname)
print "%s address is %s" %(hostname, addr)

timeout = socket.getdefaulttimeout()
print "default timeout is %s" %(timeout,)

timeout = 20
socket.setdefaulttimeout(timeout)

tm = socket.getdefaulttimeout()
print "new timeout is %s" %(tm,)

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
tm = s.gettimeout()
print "socket object time out is %s" %(tm, )
s.settimeout(10)
tm = s.gettimeout()
print "socket object time out is %s" %(tm,)


