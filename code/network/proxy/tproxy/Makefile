#
#	Makefile for C languages
#

SSL_PATH = /home/data/source/network/openssl-1.0.1j
SSL_LIBS = $(SSL_PATH)/libssl.a $(SSL_PATH)/libcrypto.a

CFLAGS = -Wall -O2 -I $(SSL_PATH)/include
LDFLAGS = -lpthread -ldl -static # -lssl -lcrypto

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = tproxyd
OBJECTS = ip_addr.o sock_util.o ssl_util.o objpool.o \
	  cpu_util.o fd_epoll.o thread.o task.o \
	  certset.o listener.o connection.o session.o \
	  trapt_util.o tproxy_util.o \
	  proxy_config.o svrpool.o policy.o \
	  worker.o proxy.o main.o $(SSL_LIBS)

TEST = 

.PHONY : all test clean depclean $(TARGET) $(TEST)

# for all target
all : $(TARGET)


tproxyd : $(OBJECTS) main.o
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



