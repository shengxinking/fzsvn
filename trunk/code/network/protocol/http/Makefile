#
#	rules.Make:	the global makefile
#

CC = gcc
CFLAGS = -Wall -ggdb -O0  -MD -MF
LDFLAGS =

ifneq (".deps", "$(wildcard .deps)")
	DEPS=.deps
endif


TARGET = httpcli httpsvr
TEST = http_protocol_test

.PHONY : all test clean depclean

all : $(TARGET)

httpcli : httpcli.o http_protocol.o sock.o

httpsvr : httpsvr.o http_protocol.o sock.o


test : $(TEST)

http_protocol_test : http_protocol.o http_protocol_test.o


%.o : %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) .deps/$(<:.c=.d)

$(DEPS) :
	@mkdir -p .deps

clean : 
	rm -f *.o $(TARGET) $(TEST)


depclean:
	rm -rf *.o $(TARGET) $(TEST) .deps

-include .deps/*.d

