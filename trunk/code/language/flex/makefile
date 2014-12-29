#
#	Makefile for C languages
#

CC = gcc
CFLAGS = -Wall -ggdb
LDFLAGS = 
LEX = flex
YACC = bison

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET =

TEST = wordcount echo wordsort

.PHONY : all test clean distclean

# for all target
all : $(TARGET) $(TEST)


# for test target
test : $(TEST)


wordcount : wordcount.o


echo : echo.o


wordsort : wordsort.o



# for clean target
clean :
	rm -f *.o $(TARGET) $(TEST)


# for distclean target
distclean:
	rm -rf *.o $(TARGET) $(TEST) .deps


# general rules for compiler
%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< -MD -MF .deps/$(<:.c=.d)

%.c : %.lex 
	$(LEX) -o $@ $<

$(DEPS) :
	@mkdir -p .deps


-include .deps/*.d


