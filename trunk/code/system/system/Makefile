#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
# LDFLAGS = -static

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = cpu_usage daemon_test lowmem_usage rt_signal 

TEST = daemon_test

.PHONY : all test clean distclean

# for all target
all : $(TARGET)

cpu_usage: cpu_usage.o


# for test target
test : $(TEST)


daemon_test : daemon.o daemon_test.o


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


