# write by jbug

CC=gcc
CFLAGS=-g -Wall

all: p3.5.o, p6.1.o

p3.5.o: p3.5.c
	$(CC) -c p3.5.c $(CFLAGS)

p6.1.o: p6.1.c
	$(CC) -o p6.1.o -c p6.1.c $(CFLAGS)

p8.3.o: p8.3.c
	$(CC) -o p8.3.o -c p8.3.c $(CFLAGS)
    
.PHONY:
clean:
	rm *.o
