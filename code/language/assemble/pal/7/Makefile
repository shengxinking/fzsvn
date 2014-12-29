#
#	Makefile for assemble language
#

TARGET = inttest movzxtest movsxtest quadtest mmxtest

.PHONY : all clean distclean

all : $(TARGET)


inttest : inttest.o
	$(LD) -o $@ $^

movzxtest : movzxtest.o
	$(LD) -o $@ $^

movsxtest : movsxtest.o
	$(LD) -o $@ $^

quadtest : quadtest.o
	$(LD) -o $@ $^

mmxtest : mmxtest.o
	$(LD) -o $@ $^

calltest : calltest.o
	$(LD) -o $@ $^ $(LDFLAGS)






clean :
	rm -f *.o $(TARGET)

distclean :
	rm -rf *.o $(TARGET) .deps

include ../rules.Make

