#
#	Makefile:	the global makefile
#

CC = gcc
CFLAGS = -Wall -ggdb  -MD -MF
LDFLAGS = -lpthread

ifneq (".deps", "$(wildcard .deps)")
	DEPDIR=.deps
endif

TARGET = shmpool_test dashboard_test

.PHONY : all clean

all : $(TARGET)

shmpool_test : shmpool.o shmpool_test.o

dashboard_test : dashboard.o dashboard_test.o

%.o : %.c $(DEPDIR)
	$(CC) -c -o $@ $< $(CFLAGS) .deps/$(<:.c=.d)

$(DEPDIR) :
	@mkdir -p .deps

clean : 
	rm -rf *.o $(TARGET) .deps

-include .deps/*.d
