#
#	Makefile for proxy
#

CC = gcc
CFLAGS = -Wall -ggdb -Wsign-compare
LDFLAGS = -lpthread -static

TARGET = proxyd # proxy1 proxy2 proxy3
OBJS = ip_addr.o sock_util.o objpool.o session.o packet.o thread.o \
       nb_splice.o

ifneq (".deps", "$(wildcard .deps)")
	DEPDIR = .deps
endif


.PHONY : all clean depclean


all : $(TARGET)

# the first proxy, using accept, work thread.
proxyd : $(OBJS) accept.o work.o proxy.o main.o 
	$(CC) -o $@ $^ $(LDFLAGS)

# the second proxy, using accept1, recv1, work1, send1 thread.
proxyd1 : $(OBJS) proxy1.o work1.o accept1.o recv1.o send1.o


# the third proxy, using accept2, work2, send2 thread.
proxyd2 : $(OBJS) proxy2.o work2.o accept2.o send2.o


# the fourth proxy, using proxy3, accept3, work3 thread.
proxyd3 : $(OBJS) proxy3.o work3.o accept3.o


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


