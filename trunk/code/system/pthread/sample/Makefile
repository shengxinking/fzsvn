#
#	Makefile:	pthread
#

CC = gcc
CFLAGS = -ggdb -Wall
LDFLAGS = -lpthread

TARGET = atexit signal exit threadid

.PHONY: all clean

all : $(TARGET)

atexit : atexit.o

clean :
	rm -f *.o atexit
