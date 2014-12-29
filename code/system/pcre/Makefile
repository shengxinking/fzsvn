#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -O3 
LDFLAGS = 

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET =

TEST = pcreperf_test_gcc pcreperf_test_icc

.PHONY : all test clean distclean

# for all target
all : $(TARGET)


# for test target
test : $(TEST)

pcreperf_test_gcc.o : pcreperf_test.c $(DEPS)
	gcc $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.c=.d)

pcreperf_test_icc.o : pcreperf_test.c $(DEPS)
	icc $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.c=.d)

pcreperf_test_gcc : pcreperf_test_gcc.o 
	gcc -o $@ $^ -lpcre

pcreperf_test_icc : pcreperf_test_icc.o libpcre-icc.a
	icc -o $@ $^

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


