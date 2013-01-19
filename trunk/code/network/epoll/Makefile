#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = -lpthread -lm

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = epwbench epoll_test


.PHONY : all clean distclean

all : $(TARGET)

epwbench : epwbench.o
	$(CC) -o $@ $^ $(LDFLAGS)

epoll_test : epoll_test.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean :
	rm -f *.o $(TARGET)


distclean:
	rm -rf *.o $(TARGET) .deps


%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.s=.d)

$(DEPS) :
	@mkdir -p .deps


-include $(DEPS)/*.d

