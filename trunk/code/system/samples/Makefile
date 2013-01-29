#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb -MD -MF
LDFLAGS = 

ifneq (".deps", "$(wildcard .deps)")
DEPS = .deps
endif

TARGET = static_o2_test static_o0_test


.PHONY : all clean depclean

all : $(TARGET)

static_o2_test : static_o2_test.c
	$(CC) -O2 -c -o static_o2_test.o static_o2_test.c
	$(CC) -o static_o2_test static_o2_test.o

static_o0_test : static_o2_test.c
	$(CC) -O0 -c -o static_o0_test.o static_o2_test.c
	$(CC) -o static_o0_test static_o0_test.o

clean :
	rm -f *.o $(TARGET)


depclean:
	rm -rf *.o $(TARGET) .deps




%.o : %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) .deps/$(<:.c=.d)

$(DEPS) :
	@mkdir -p .deps


-include $(DEPS)/*.d

