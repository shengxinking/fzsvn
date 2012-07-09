#!/usr/bin/env python

import socket, traceback, sys


def __usage():
    """ print help infomation """
    print "echosvr_timeout.py <port> <timeout>"

def __parse_cmd():
    if len(sys.argv) != 3:
        return (-1, -1)
    
    try:
        port = int(sys.argv[1])
        timeout = int(sys.argv[2])
    except ValueError, e:
        print "value error: " + str(e)
        return (-1, -1)
    return (port, timeout)


def __tcpsvr(port):
    ''' Create a TCP listen socket and return it '''

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(('', port))
        s.listen(1024)
    except:
        traceback.print_exc()
        sys.exit(1)
        
    return s


def __do_loop(s, timeout):
    ''' loop to accept client and recv data and echo data '''

    while 1:
        
        try:
            cli, cliaddr = s.accept()
        except KeyboardInterrupt:
            raise
        except:
            traceback.print_exc()
            continue

        cli.settimeout(timeout)
        
        # recv data and echo data
        try:
            print "client %s:%s connected" % cliaddr

            while 1:
                data = cli.recv(4096)
                if not len(data):
                    break
                cli.sendall(data)
        except (KeyboardInterrupt, SystemExit):
            raise
        except socket.timeout:
            print "client %s:%s timeout" % cliaddr
            pass
        except:
            traceback.print_exc()

        # close socket 
        try:
            cli.close()
        except KeyboardInterrupt:
            raise
        except:
            traceback.print_exc()

if __name__ == "__main__":
    ''' main entry '''
    port, timeout = __parse_cmd()
    if port == -1:
        __usage()
        sys.exit(-1)

    s = __tcpsvr(port)
    __do_loop(s, timeout)

    
