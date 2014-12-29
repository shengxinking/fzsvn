#
#
#


TARGET = cpuid cpuid2

all : $(TARGET)

cpuid : cpuid.o
	$(LD) -o $@ $^

cpuid2 : cpuid2.o
	$(LD) -o $@ $^ $(LDFLAGS)

clean:
	rm *.o $(TARGET) -f

distclean:
	rm *.o $(TARGET) .deps -rf

include ../rules.Make
