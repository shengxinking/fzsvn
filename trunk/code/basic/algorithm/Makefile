#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -O2 
LDFLAGS = 

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = algorithm_test

TEST = 

.PHONY : all test clean depclean

# for all target
all : $(TARGET)

algorithm_test : bsearch.o BM_search.o gcd.o algorithm_test.o

# for test target
test : $(TEST)


# for clean target
clean :
	rm -f *.o $(TARGET)


# for depclean target
depclean:
	rm -rf *.o $(TARGET) .deps


%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.s=.d)

$(DEPS) :
	@mkdir -p .deps


-include .deps/*.d


