#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = 

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = 

TEST = str_bm_test

.PHONY : all test clean depclean

# for all target
all : $(TARGET)


# for test target
test : $(TEST)


# for clean target
clean :
	rm -f *.o $(TARGET)

strbm_test : strbm.o strbm_test.o



# for depclean target
depclean:
	rm -rf *.o $(TARGET) .deps


%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.s=.d)

$(DEPS) :
	@mkdir -p .deps


-include .deps/*.d


