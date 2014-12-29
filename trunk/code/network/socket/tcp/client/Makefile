#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = -lpthread

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = tcpcli_thread tcpcli_tw

TEST = 

.PHONY : all test clean distclean

# for all target
all : $(TARGET)

tcpcli_tw : sock.o tcpcli_tw.o

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


