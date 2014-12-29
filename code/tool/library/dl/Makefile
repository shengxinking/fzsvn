#
#	Makefile of dl
#

CC = gcc
CFLAGS = -Wall -ggdb

TARGET = t1
OBJS   = t1.o
LIBS   = -ldl

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

clean :
	rm -f $(TARGET) $(OBJS)