#
#	Makefile	makefile for CLI(command line interface)
#

CC = gcc
CFLAGS = -ggdb -Wall -MD -MF
LDFLAGS = -lreadline

ifneq (".deps", "$(wildcard .deps)")
	DEPDIR=.deps
endif

TARGET = cmfsvrd
TEST = 

OBJECT = libcli.o

.PHONY : all test clean depclean

all : $(TARGET) 

cli : cli_main.o cli.o object.o perm.o command.o
	$(CC) -o $@ $^ $(LDFLAGS)

cmfsvrd : cmf_shm.o cmfsvr_main.o
	$(CC) -o $@ $^ $(LDFLAGS)

test : $(TEST)


clean : 
	rm -rf *.o $(TARGET) $(TEST) $(OBJECT)

depclean :
	rm -rf *.o $(TARGET) $(TEST) $(OBJECT) .deps


%.o : %.c $(DEPDIR)
	$(CC) -c -o $@ $< $(CFLAGS) .deps/$(<:.c=.d)

$(DEPDIR) :
	@mkdir -p .deps

-include .deps/*.d

