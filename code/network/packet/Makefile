#
#	Makefile for C languages
#

SSL_PATH = /home/data/source/network/openssl-1.0.1j
SSL_LIBS = $(SSL_PATH)/libssl.a $(SSL_PATH)/libcrypto.a

CC = gcc
CFLAGS = -Wall -ggdb -I $(SSL_PATH)/include
LDFLAGS = -lpcap -lpthread -ldl -lz

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = sniffex ssldump

TEST = 

.PHONY : all test clean depclean

# for all target
all : $(TARGET)

sniffex : sniffex.o \
	  packet_eth.o \
	  packet_arp.o \
	  packet_ipv4.o \
	  packet_ipv6.o \
	  packet_icmpv4.o \
	  packet_icmpv6.o \
	  packet_udp.o \
	  packet_tcp.o

ssldump : ssldump.o \
	  packet_eth.o \
	  packet_ipv4.o \
	  packet_ipv6.o \
	  packet_tcp.o \
	  ip_addr.o \
	  objpool.o \
	  tcp_stream.o \
	  dssl_util.o \
	  $(SSL_LIBS)

# for test target
test : $(TEST)


# for clean target
clean :
	rm -f *.o $(TARGET) $(TEST)


# for depclean target
depclean:
	rm -rf *.o $(TARGET) $(TEST) .deps


%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< -MMD -MF .deps/$(<:.c=.d)

$(DEPS) :
	@mkdir -p .deps


-include .deps/*.d



