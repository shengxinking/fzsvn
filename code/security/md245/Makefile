#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = -lcrypto

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = md2 md4 md5 sha sha1

TEST = 

.PHONY : all test clean distclean

# for all target
all : $(TARGET)

md2 : md2_main.o
	$(CC) -o $@ $^ $(LDFLAGS)

md4 : md4_main.o
	$(CC) -o $@ $^ $(LDFLAGS)

md5 : md5_main.o
	$(CC) -o $@ $^ $(LDFLAGS)

sha : sha_main.o
	$(CC) -o $@ $^ $(LDFLAGS)

sha1 : sha1_main.o
	$(CC) -o $@ $^ $(LDFLAGS)


# for test target
test : $(TEST)


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


