#
#	Makefile	makefile for CLI(command line interface)
#

CC = gcc
CFLAGS = -ggdb -Wall -MD -MF
LDFLAGS = -lreadline

ifneq (".deps", "$(wildcard .deps)")
	DEPDIR=.deps
endif

TARGET = cli clisvr
TEST = object_test cli_test

OBJECT = libcli.o

.PHONY : all test clean distclean

all : $(TARGET) 

cli : cli_main.o cli.o object.o perm.o command.o
	$(CC) -o $@ $^ $(LDFLAGS)

clisvr : clisvr_main.o object.o perm.o command.o plugin.o sys_global.o
	$(CC) -o $@ $^ $(LDFLAGS)

test : $(TEST)

object_test : object.o object_test.o

cli_test : cli_test.o command.o cli.o libcli.o plugin.o sys_global.o object.o

libcli.o : object.o perm.o query.o 
	$(AR) -r $@ $^

clean : 
	rm -rf *.o $(TARGET) $(TEST) $(OBJECT)

distclean :
	rm -rf *.o $(TARGET) $(TEST) $(OBJECT) .deps


%.o : %.c $(DEPDIR)
	$(CC) -c -o $@ $< $(CFLAGS) .deps/$(<:.c=.d)

$(DEPDIR) :
	@mkdir -p .deps

-include .deps/*.d

