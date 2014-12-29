#!/usr/bin/env python

import socket

def tcpsvr(port):
    """Create a tcp server on port(port)"""
    import socket
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(('', port))
    s.listen(1024)
    return s

def tcpsvr_loop(s):
    '''recv data from client, print it'''
    while 1:
        cli, cliaddr = s.accept();
        clifile = cli.makefile("rw", 0)
        clifile.write("Welcome, " + str(cliaddr) + "\n")
        clifile.write("please input a string: ")
        msg = clifile.read(100)
        print "client: " + msg
        clifile.close()
        cli.close()

if __name__ == "__main__":
    s = tcpsvr(8088)
    tcpsvr_loop(s)
