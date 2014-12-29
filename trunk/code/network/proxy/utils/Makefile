#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -ggdb -Wall
LDFLAGS = -lpthread

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET =  

TEST = session_test

.PHONY : all test clean depclean

# for all target
all : $(TARGET)


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


