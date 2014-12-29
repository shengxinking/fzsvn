#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = -static

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = showfile

TEST = memperf showfile ioperf filedes_limit socket_limit

.PHONY : all test clean distclean

# for all target
all : $(TARGET)

showfile : showfile.o
	$(CC) -o $@ $^ $(LDFLAGS)

# for test target
test : $(TEST)

memperf : memperf.o
	$(CC) -o $@ $^ $(LDFLAGS)

ioperf : ioperf.o
	$(CC) -o $@ $^ $(LDFLAGS)

filedes_limit : filedes_limit.o
	$(CC) -o $@ $^ $(LDFLAGS)

socket_limit : socket_limit.o
	$(CC) -o $@ $^ $(LDFLAGS)


# for clean target
clean :
	rm -f *.o $(TARGET) $(TEST)

# for distclean target
distclean:
	rm -rf *.o $(TARGET) $(TEST) .deps


%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.s=.d)

$(DEPS) :
	@mkdir -p .deps


-include .deps/*.d



