#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = -lpthread -static

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = netopt_test

TEST = 

.PHONY : all test clean depclean

# for all target
all : $(TARGET)

netopt_test : cpu_util.o net_procfs.o net_sysfs.o netopt_test.o


# for test target
test : $(TEST)


# for clean target
clean :
	rm -f *.o $(TARGET) $(TEST)


# for depclean target
depclean:
	rm -rf *.o $(TARGET) $(TEST) .deps


%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.c=.d)

$(DEPS) :
	@mkdir -p .deps


-include .deps/*.d



