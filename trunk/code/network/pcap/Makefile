#
#  Makefile of pcap program
#

CC = gcc
CFLAGS = -Wall -Wstrict-prototypes

LIBS = /usr/lib/libpcap.a

TARGETS = 1

all : $(TARGETS)

$(TARGETS) : % : %.c
	$(CC) $(CFLAGS) -o $@ $<  $(LIBS)

clean :
	rm -f $(TARGETS)

