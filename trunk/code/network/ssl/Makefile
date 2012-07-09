#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = -lssl -lcrypto -lpthread

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = sslcli sslcli_thread sslcli_echo \
	 sslsvr sslsvr_select sslsvr_poll \
	 sslverify sslciphers

TEST = sslsock_test

.PHONY : all test clean distclean

# for all target
all : $(TARGET)

sslcli : sslcli.o sock.o sslsock.o

sslcli_thread : sslcli_thread.o sock.o sslsock.o

sslcli_echo : sslcli_echo.o

sslsvr : sslsvr.o sock.o sslsock.o

sslsvr_select : sslsvr_select.o

sslsvr_poll : sslsvr_poll.o

sslverify : sslverify.o sslcommon.o

sslpool_test : sslpool.o sslpool_test.o

sslciphers : sslciphers.o

# for test target
test : $(TEST)

sslsock_test : sock.o sslsock.o sslsock_test.o


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


