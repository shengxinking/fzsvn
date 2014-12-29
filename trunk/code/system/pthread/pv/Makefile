#
#	Makefile
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = -lpthread

TARGET = pv_mutex pv_rwlock

.PHONY: all clean

all: $(TARGET)

pv_mutex : pv_mutex.o

clean:
	rm -f *.o $(TARGET)
