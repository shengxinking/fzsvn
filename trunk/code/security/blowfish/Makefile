#
#	Makefile of blowfish cipher test program
#

CC = gcc
CFLAGS = -ggdb -Wall -Wstrict-prototypes

TARGET = blowfish
OBJS = blowfish.o
LIBS = -lcrypto

all : $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) -o $@ $< $(LIBS)

clean:
	rm *.o $(TARGET)