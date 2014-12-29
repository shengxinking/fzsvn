#
#	Makefile for assemble language
#

AS = as
LD = ld
NASM = nasm
AS_FLAGS = --gstabs
NASM_FLAGS = -f elf

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif

TARGET = hello hello_1 movtest

.PHONY : all clean distclean

all : $(TARGET)


hello : hello.o
	ld -s -o $@ $^

hello_1 : hello_1.o
	ld -s -o $@ $^

movtest : movtest.o
	ld -s -o $@ $^


clean:
	rm -f *.o $(TARGET)


distclean :
	rm -rf *.o $(TARGET) .deps



%.o : %.s $(DEPS)
	$(AS) $(AS_FLAGS) -o $@ $< --MD .deps/$(<:.s=.d)

%.o : %.asm
	$(NASM) $(NASM_FLAGS) -o $@ $< 

$(DEPS) :
	@mkdir -p .deps


-include $(DEPS)/*.d

