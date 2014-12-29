#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = -lreadline

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = 

TEST = complete_test bindkey_test

.PHONY : all test clean distclean

# for all target
all : $(TARGET)


# for test target
test : $(TEST)

complete_test : complete_test.o

bindkey_test : bindkey_test.o

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


