#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = 

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = sock_util_test ip_addr_test


.PHONY : all clean depclean

all : $(TARGET)


sock_util_test : ip_addr.o sock_util.o sock_util_test.o
	$(CC) -o $@ $^ $(LDFLAGS)
	

ip_addr_test : ip_addr.o ip_addr_test.o
	$(CC) -o $@ $^ $(LDFLAGS)

clean :
	rm -f *.o $(TARGET)


depclean:
	rm -rf *.o $(TARGET) .deps


%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.s=.d)

$(DEPS) :
	@mkdir -p .deps


-include $(DEPS)/*.d

