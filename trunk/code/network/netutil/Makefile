#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = 

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = netutil_test


.PHONY : all clean distclean

all : $(TARGET)


netutil_test : ip_addr.o sock_util.o netutil_test.o
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

