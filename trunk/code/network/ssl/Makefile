#
#	Makefile for C languages
#

SSL_PATH = /home/data/source/network/openssl-1.0.1j
SSL_LIBS = $(SSL_PATH)/libssl.a $(SSL_PATH)/libcrypto.a

CC = gcc
CFLAGS = -Wall -ggdb -I $(SSL_PATH)/include
LDFLAGS = -lpthread -ldl -lz

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = ssl_client ssl_server

.PHONY : all test clean depclean

# for all target
all : $(TARGET)

ssl_client : ip_addr.o sock_util.o ssl_util.o ssl_client.o $(SSL_LIBS)
	$(CC) -o $@ $^ $(LDFLAGS)


ssl_server : ip_addr.o sock_util.o ssl_util.o ssl_server.o $(SSL_LIBS)
	$(CC) -o $@ $^ $(LDFLAGS)

# for test target
test : $(TEST)

# for clean target
clean :
	rm -f *.o $(TARGET) $(TEST)


# for depclean target
depclean:
	rm -rf *.o $(TARGET) $(TEST) .deps


%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.c=.d)

$(DEPS) :
	@mkdir -p .deps


-include .deps/*.d


