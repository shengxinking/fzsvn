#
#	Makefile for libXML SAX2 parse
#

CC = gcc
CFLAGS = -I/usr/include/libxml2
LDFLAGS = -L/usr/lib -lxml2

ifneq (".deps", "$(wildcard .deps)")
	DEPDIR = .deps
endif


TARGET = saxtest domtest

.PHONY : all clean distclean

all : $(TARGET)

saxtest : saxtest.o
	$(CC) -o $@ $^ $(LDFLAGS)

domtest : domtest.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean :
	rm -f *.o $(TARGET)

distclean:
	rm -rf *.o $(TARGET) .deps



# depend file and compile rules

%.o : %.c $(DEPDIR)
	$(CC) -c -o $@ $< $(CFLAGS) -MD -MF .deps/$(<:.c=.d)

$(DEPDIR) :
	mkdir -p .deps

-include .deps/*.d
