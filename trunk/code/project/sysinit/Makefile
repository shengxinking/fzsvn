#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = 

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = init smit

TEST = 

.PHONY : all test clean distclean

# for all target
all : $(TARGET)

init : init.o util.o
	$(CC) -o $@ $^ -static -lrt


smit : smit.o filesys.o hardware.o network.o system.o util.o
	$(CC) -o $@ $^ -static


# for test target
test : $(TEST)


# for clean target
clean :
	rm -f *.o $(TARGET) $(TEST)

# for distclean target
distclean:
	rm -rf *.o $(TARGET) $(TEST) .deps

%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.c=.d)

$(DEPS) :
	@mkdir -p .deps


-include .deps/*.d


