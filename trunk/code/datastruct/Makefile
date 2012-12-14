#
#	Makefile:	the global makefile
#

CC = gcc
CFLAGS = -Wall -ggdb  -MD -MF
LDFLAGS =

ifneq (".deps", "$(wildcard .deps)")
	DEPDIR=.deps
endif

TARGET = kth_test maxsubseqsum_test mempool_test

.PHONY : all clean

all : $(TARGET)

kth_test : kth.o kth_test.o

maxsubseqsum_test: maxsubseqsum.o maxsubseqsum_test.o

mempool_test : mempool.o mempool_test.o
	$(CC) -o $@ $^ $(LDFLAGS) -lpthread

%.o : %.c $(DEPDIR)
	$(CC) -c -o $@ $< $(CFLAGS) .deps/$(<:.c=.d)

$(DEPDIR) :
	@mkdir -p .deps

clean : 
	rm -rf *.o $(TARGET) .deps

-include .deps/*.d
