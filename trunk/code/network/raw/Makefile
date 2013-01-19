#
#	Makefile for C languages
#

CC = /fortidev2/bin/fortigcc
CFLAGS = -Wall -ggdb
LDFLAGS = 

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET =

TEST = rawproxy

.PHONY : all test clean distclean

# for all target
all : $(TARGET)


# for test target
test : $(TEST)


rawproxy : rawproxy.o
	$(CC) -o $@ $^ $(LDFLAGS)

# for clean target
clean :
	rm -f *.o $(TARGET)


# for distclean target
distclean:
	rm -rf *.o $(TARGET) $(TEST) .deps


%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.s=.d)

$(DEPS) :
	@mkdir -p .deps


-include .deps/*.d


