#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = 

ifneq (".deps", "$(wildcard .deps)")
DEPS = .deps
endif

TARGET = static_o2_test static_o0_test static_inline_test_o2 \
	 static_inline_test_o0 gzip_test malloc_test


.PHONY : all clean depclean

all : 


clean :
	rm -f *.o $(TARGET)


depclean:
	rm -rf *.o $(TARGET) .deps




%.o : %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) -MD -MF .deps/$(<:.c=.d)

$(DEPS) :
	@mkdir -p .deps


-include $(DEPS)/*.d

