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

TEST = asm_test1 asm_test2 asm_test3

.PHONY : all test clean distclean

# for all target
all : $(TARGET)


# for test target
test : $(TEST)

asm_test1 : asm_test1.o
	$(CC) -o $@ $^ $(LDFLAGS)

asm_test2 : asm_test2.o
	$(CC) -o $@ $^ $(LDFLAGS)

asm_test3 : asm_test3.o
	$(CC) -o $@ $^ $(LDFLAGS)

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


