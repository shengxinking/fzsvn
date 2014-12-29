#
#	Makefile for assemble language
#

TARGET = sizetest1 sizetest2 sizetest3 movtest1 movtest2 movtest3 \
	 movtest4 cmovtest xchgtest swaptest xaddtest cmpxchgtest \
         cmpxchg8btest bubble stacktest

.PHONY : all clean distclean

all : $(TARGET)


sizetest1 : sizetest1.o
	$(LD) -o $@ $<

sizetest2 : sizetest2.o
	$(LD) -o $@ $<

sizetest3 : sizetest3.o
	$(LD) -o $@ $^

movtest1 : movtest1.o
	$(LD) -o $@ $<

movtest2 : movtest2.o
	$(LD) -o $@ $<

movtest3 : movtest3.o
	$(LD) $(LDFLAGS) -o $@ $<

movtest4 : movtest4.o
	$(LD) $(LDFLAGS) -o $@ $<

cmovtest : cmovtest.o
	$(LD) $(LDFLAGS) -o $@ $^

xchgtest : xchgtest.o
	$(LD) -o $@ $^

swaptest : swaptest.o
	$(LD) -o $@ $^

xaddtest : xaddtest.o
	$(LD) -o $@ $^

cmpxchgtest : cmpxchgtest.o
	$(LD) -o $@ $^

cmpxchg8btest : cmpxchg8btest.o
	$(LD) -o $@ $^

bubble : bubble.o
	$(LD) -o $@ $^

stacktest : stacktest.o
	$(LD) -o $@ $^

clean :
	rm -f *.o $(TARGET)

distclean :
	rm -rf *.o $(TARGET) .deps

include ../rules.Make

