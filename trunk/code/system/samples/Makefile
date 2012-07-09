#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb -MD -MF
LDFLAGS = 

ifneq (".deps", "$(wildcard .deps)")
DEPS = .deps
endif

TARGET = backtrace segfault coredump sysctl


.PHONY : all clean distclean

all : $(TARGET)


backtrace : backtrace.o
	$(CC) -o $@ $^ -rdynamic $(LDFLAGS)


segfault : segfault.o
	$(CC) -o $@ $^ $(LDFLAGS)


coredump : coredump.o
	$(CC) -o $@ $^ $(LDFLAGS)

sysctl : sysctl.o
	$(CC) -o $@ $^ 

clean :
	rm -f *.o $(TARGET)


distclean:
	rm -rf *.o $(TARGET) .deps






%.o : %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) .deps/$(<:.c=.d)

$(DEPS) :
	@mkdir -p .deps


-include $(DEPS)/*.d

