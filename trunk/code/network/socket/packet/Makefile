#
#  Makefile of raw socket receive function
#

CC     = gcc
CFLAGS = -Wall -Wstrict-prototypes -ggdb

TARGET = rawrecv
OBJS   = rawrecv.o

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $@ $^

clean :
	rm -rf $(TARGET) $(OBJS)
