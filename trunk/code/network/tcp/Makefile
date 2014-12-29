#
#	Makefile for proxy
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = -lpthread -static

TARGET = tcp_client tcp_server

ifneq (".deps", "$(wildcard .deps)")
	DEPDIR = .deps
endif


.PHONY : all clean depclean test


all : $(TARGET)

test : $(TEST)

clean :
	rm -f *.o $(TARGET) $(TEST)

depclean : 
	rm -rf *.o $(TARGET) $(TEST) .deps 

tcp_client: tcp_client.o ip_addr.o sock_util.o


tcp_server: tcp_server.o ip_addr.o sock_util.o


%.o : %.c $(DEPDIR)
	$(CC) -c -o $@ $< $(CFLAGS) -MD -MF .deps/$(<:.c=.d)

$(DEPDIR) :
	mkdir -p .deps

-include .deps/*.d


