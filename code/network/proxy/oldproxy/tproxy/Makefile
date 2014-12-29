#
#	Makefile for proxy
#

CC = gcc
CFLAGS = -Wall -O2 -Wsign-compare
LDFLAGS = -lpthread -static

TARGET = tproxyd
OBJS = ip_addr.o cpu_util.o sock_util.o objpool.o \
       thread.o fd_epoll.o task.o

ifneq (".deps", "$(wildcard .deps)")
	DEPDIR = .deps
endif


.PHONY : all clean depclean


all : $(TARGET)

# proxyd, using 1 accept, many work thread(2 epoll).
tproxyd : $(OBJS) worker.o proxy.o main.o 
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


