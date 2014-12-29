#
#	Makefile for proxy
#

CC = gcc
CFLAGS = -Wall -O2 -I../utils/
LDFLAGS = -lpthread -static

TARGET = proxyd1 proxyd2 # proxyd_event
OBJS = ip_addr.o sock_util.o cpu_util.o nb_splice.o objpool.o \
       session.o fd_epoll.o packet.o childproc.o


ifneq (".deps", "$(wildcard .deps)")
	DEPDIR = .deps
endif


.PHONY : all clean distclean

all : $(TARGET)

proxyd : $(OBJS) proxy.o main.o
	$(CC) -o $@ $^ $(LDFLAGS)


proxyd1 : $(OBJS) proxy1.o main.o
	$(CC) -o $@ $^ $(LDFLAGS)

proxyd2: $(OBJS) proxy2.o main.o
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


