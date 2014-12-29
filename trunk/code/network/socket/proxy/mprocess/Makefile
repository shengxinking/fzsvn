#
#	Makefile for proxy
#

CC = gcc
CFLAGS = -Wall -ggdb -I../utils/
LDFLAGS = -lpthread -static

TARGET = proxyd # proxyd_event
OBJS = ip_addr.o sock_util.o nb_splice.o objpool.o \
       session.o packet.o childproc.o


ifneq (".deps", "$(wildcard .deps)")
	DEPDIR = .deps
endif


.PHONY : all clean distclean

all : $(TARGET)

proxyd : $(OBJS) proxy.o main.o
	$(CC) -o $@ $^ $(LDFLAGS)


proxyd_event : $(OBJS) epoll_event.o proxy_event.o main.o
	$(CC) -o $@ $^ $(LDFLAGS)


clean :
	rm -f *.o $(TARGET) 

depclean : 
	rm -rf *.o $(TARGET) .deps 



%.o : %.c $(DEPDIR)
	$(CC) $(CFLAGS) -c -o $@ $< $(CFLAGS) -MD -MF .deps/$(<:.c=.d)

$(DEPDIR) :
	mkdir -p .deps

-include .deps/*.d


