#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = 

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

OBJS = dbg_file.o dbg_print.o dbg_segfault.o


TARGET = debug_test

.PHONY : all clean distclean

# for all target
all : $(TARGET)


# for test target
test : $(TEST)

debug_test : debug.o debug_test.o
	$(CC) -o $@ $^ $(LDFLAGS)

dbg_file_test : dbg_file.o dbg_file_test.o
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


