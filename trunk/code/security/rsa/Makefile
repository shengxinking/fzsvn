##
#
#



CC = gcc
CFLAGS = -ggdb -O0 -Wall
LDFLAGS = -lcrypto


.PHONY: all clean


all : rsa_test


rsa_test : rsa_test.o keyfile.o
	$(CC) -o $@ $^ $(LDFLAGS)


clean:
	rm -f *.o rsa_test