#
#	Makefile for proxy
#

CC = gcc
CFLAGS = -Wall -O2 -Wsign-compare
LDFLAGS = -lpthread -static

TARGET = pproxyd
OBJS = ip_addr.o cpu_util.o sock_util.o objpool.o \
       childproc.o fd_epoll.o task.o

ifneq (".deps", "$(wildcard .deps)")
	DEPDIR = .deps
endif


.PHONY : all clean depclean


all : $(TARGET)

pproxyd : $(OBJS) main.o proxy.o worker.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean :
	rm -f *.o $(TARGET) $(TEST)

depclean : 
	rm -rf *.o $(TARGET) $(TEST) .deps 

test : $(TEST)

%.o : %.c $(DEPDIR)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.c=.d)

$(DEPDIR) :
	mkdir -p .deps

-include .deps/*.d


