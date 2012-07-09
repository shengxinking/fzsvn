#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = -lrt

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = msgecho msgcli mqecho

TEST = 

.PHONY : all test clean distclean

# for all target
all : $(TARGET)


msgecho : msgecho.o
	$(CC) -o $@ $^ $(LDFLAGS)

msgcli : msgcli.o
	$(CC) -o $@ $^ $(LDFLAGS)

mqecho : mqecho.o
	$(CC) -o $@ $^ $(LDFLAGS)

# for test target
test : $(TEST)


# for clean target
clean :
	rm -f *.o $(TARGET)


# for distclean target
distclean:
	rm -rf *.o $(TARGET) .deps


%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.s=.d)

$(DEPS) :
	@mkdir -p .deps


-include .deps/*.d


