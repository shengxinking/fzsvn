#
#	Makefile for assemble language
#

TARGET = jmptest calltest cmptest jotest paritytest signtest \
	 looptest looptest1

.PHONY : all clean distclean

all : $(TARGET)


jmptest : jmptest.o
	$(LD) -o $@ $^

calltest : calltest.o
	$(LD) -o $@ $^ $(LDFLAGS)

cmptest : cmptest.o
	$(LD) -o $@ $^ $(LDFLAGS)


jotest : jotest.o
	$(LD) -o $@ $^

paritytest : paritytest.o
	$(LD) -o $@ $^

signtest : signtest.o
	$(LD) -o $@ $^ $(LDFLAGS)

looptest : looptest.o
	$(LD) -o $@ $^ $(LDFLAGS)

looptest1 : looptest1.o
	$(LD) -o $@ $^ $(LDFLAGS)


clean :
	rm -f *.o $(TARGET)

distclean :
	rm -rf *.o $(TARGET) .deps

include ../rules.Make

