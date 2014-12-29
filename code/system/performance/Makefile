#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -O2
# LDFLAGS = -static

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = syscall_test memcpy_test clflush_test

TEST = 

.PHONY : all test clean distclean

# for all target
all : $(TARGET)

syscall_test: syscall_test.o

memcpy_test: memcpy_test.o

clflush_test : clflush_test.o

# for clean target
clean :
	rm -f *.o $(TARGET) $(TEST)


# for distclean target
depclean:
	rm -rf *.o $(TARGET) $(TEST) .deps


%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.c=.d)

$(DEPS) :
	@mkdir -p .deps


-include .deps/*.d


