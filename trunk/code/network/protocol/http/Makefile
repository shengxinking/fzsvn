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
TEST = http_parse_test

.PHONY : all test clean depclean

all : $(TARGET)

httpcli : httpcli.o http_parse.o sock.o

httpsvr : httpsvr.o http_parse.o sock.o


test : $(TEST)

http_protocol_test : http_parse.o http_parse_test.o


%.o : %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) .deps/$(<:.c=.d)

$(DEPS) :
	@mkdir -p .deps

clean : 
	rm -f *.o $(TARGET) $(TEST)


depclean:
	rm -rf *.o $(TARGET) $(TEST) .deps

-include .deps/*.d

