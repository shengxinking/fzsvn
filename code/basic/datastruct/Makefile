#
#	Makefile:	the global makefile
#

CC = gcc
CFLAGS = -Wall -ggdb 
LDFLAGS = -lpthread

ifneq (".deps", "$(wildcard .deps)")
	DEPDIR=.deps
endif

TARGET = mempool_test hash_test cblist_test

.PHONY : all clean

all : $(TARGET)

mempool_test : mempool_test.o

hash_test : hash.o objpool.o hash_test.o
	$(CC) -o $@ $^ $(LDFLAGS)

cblist_test : cblist_test.o

clean:
	rm -rf *.o $(TARGET) $(TEST)

depclean : 
	rm -rf *.o $(TARGET) $(TEST) .deps

%.o : %.c $(DEPDIR)
	$(CC) -c -o $@ $< $(CFLAGS) -MMD -MF .deps/$(<:.c=.d)

$(DEPDIR) :
	@mkdir -p .deps


-include .deps/*.d
