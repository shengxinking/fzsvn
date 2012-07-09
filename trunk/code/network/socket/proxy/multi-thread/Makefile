#
#	Makefile for proxy
#

CC = gcc
CFLAGS = -Wall -ggdb -Wsign-compare
LDFLAGS = -lpthread -static

TARGET = proxy # proxy1 proxy2 proxy3
OBJS = sock.o mempool.o session.o http_protocol.o packet.o thread.o \

TEST = 

ifneq (".deps", "$(wildcard .deps)")
	DEPDIR = .deps
endif


.PHONY : all clean distclean test


all : $(TARGET)

# the first proxy, using accept, work thread.
proxy : $(OBJS) main.o proxy.o accept.o work_epoll.o 
	$(CC) -o $@ $^ $(LDFLAGS)

# the second proxy, using accept1, recv1, work1, send1 thread.
proxy1 : $(OBJS) proxy1.o work1.o accept1.o recv1.o send1.o


# the third proxy, using accept2, work2, send2 thread.
proxy2 : $(OBJS) proxy2.o work2.o accept2.o send2.o


# the fourth proxy, using proxy3, accept3, work3 thread.
proxy3 : $(OBJS) proxy3.o work3.o accept3.o


clean :
	rm -f *.o $(TARGET) $(TEST)

distclean : 
	rm -rf *.o $(TARGET) $(TEST) .deps 

test : $(TEST)

%.o : %.c $(DEPDIR)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.c=.d)

$(DEPDIR) :
	mkdir -p .deps

-include .deps/*.d


