#
#	Makefile for chapter 11
#

.PHONY : all clean distclean

TARGET = functest1 functest2 functest3 functest4

all : $(TARGET)

functest1 : functest1.o
	ld -o $@ $<

functest2 : functest2.o
	ld -o $@ $<

functest3 : functest3.o
	ld -o $@ $<

functest4 : functest4.o area.o
	ld -o $@ $^

clean :
	rm -f *.o $(TARGET)

distclean:
	rm -rf *.o $(TARGET) .deps


include ../rules.Make
