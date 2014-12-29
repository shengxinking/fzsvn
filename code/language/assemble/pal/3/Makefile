#
#	Makefile for chapter 3
#

TARGET = ctest ctest.s


.PHONY : all clean distclean
all : $(TARGET)

ctest: ctest.o

ctest.s :ctest.c
	$(CC) -S $<

clean:
	rm -f *.o $(TARGET)

distclean:
	rm -rf *.o $(TARGET) .deps

include ../rules.Make