#
#	Makefile for proxy
#

CC = gcc
CFLAGS = -Wall -ggdb -O3 -I../utils/
LDFLAGS = -lpthread

TARGET = proxy
OBJS = sock.o mempool.o session.o packet.o proxy.o main.o

ifneq (".deps", "$(wildcard .deps)")
	DEPDIR = .deps
endif


.PHONY : all clean distclean

all : $(TARGET)


clean :
	rm -f *.o $(TARGET) 

depclean : 
	rm -rf *.o $(TARGET) .deps 


proxy : $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)


%.o : %.c $(DEPDIR)
	$(CC) $(CFLAGS) -c -o $@ $< $(CFLAGS) -MD -MF .deps/$(<:.c=.d)

$(DEPDIR) :
	mkdir -p .deps

-include .deps/*.d


