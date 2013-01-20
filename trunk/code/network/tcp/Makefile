#
#	Makefile for proxy
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = -lpthread

TARGET = tcpsvr_epoll tcpsvr_poll tcpsvr_select
OBJS = tcpsvr_epoll.o tcpsvr_poll.o tcpsvr_select.o

ifneq (".deps", "$(wildcard .deps)")
	DEPDIR = .deps
endif


.PHONY : all clean distclean test


all : $(TARGET)

test : $(TEST)

clean :
	rm -f *.o $(TARGET) $(TEST)

distclean : 
	rm -rf *.o $(TARGET) $(TEST) .deps 


tcpsvr_epoll : sock.o tcpsvr_epoll.o
	$(CC) -o $@ $^ $(LDFLAGS)


tcpsvr_poll : sock.o tcpsvr_poll.o
	$(CC) -o $@ $^ $(LDFLAGS)

tcpsvr_select : sock.o tcpsvr_select.o
	$(CC) -o $@ $^ $(LDFLAGS)

%.o : %.c $(DEPDIR)
	$(CC) -c -o $@ $< $(CFLAGS) -MD -MF .deps/$(<:.c=.d)

$(DEPDIR) :
	mkdir -p .deps

-include .deps/*.d


