#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb -MD -MF
LDFLAGS = 

ifneq (".deps", "$(wildcard .deps)")
DEPS = .deps
endif

TARGET = static_o2_test static_o0_test static_inline_test_o2 static_inline_test_o0


.PHONY : all clean depclean

all : $(TARGET)

static_o2_test : static_o2_test.c
	$(CC) -O2 -c -o static_o2_test.o static_o2_test.c
	$(CC) -o static_o2_test static_o2_test.o

static_o0_test : static_o2_test.c
	$(CC) -O0 -c -o static_o0_test.o static_o2_test.c
	$(CC) -o static_o0_test static_o0_test.o

static_inline_test_o2 : static_inline_test.c
	$(CC) -O1 -c -o static_inline_test_o2.o static_inline_test.c
	$(CC) -o static_inline_test_o2 static_inline_test_o2.o


static_inline_test_o0 : static_inline_test.c
	$(CC) -O0 -c -o static_inline_test_o0.o static_inline_test.c
	$(CC) -o static_inline_test_o0 static_inline_test_o0.o


clean :
	rm -f *.o $(TARGET)


depclean:
	rm -rf *.o $(TARGET) .deps




%.o : %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) .deps/$(<:.c=.d)

$(DEPS) :
	@mkdir -p .deps


-include $(DEPS)/*.d

