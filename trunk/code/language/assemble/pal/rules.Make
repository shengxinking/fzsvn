#
#	Makefile for AT&T assemble language
#

AS = as
AS_FLAGS = -gstabs

LD = ld
LDFLAGS = -dynamic-linker /lib/ld-linux.so.2 -lc

ifneq (".deps", "$(wildcard .deps)")
DEPS = .deps
endif

%.o : %.s $(DEPS)
	$(AS) $(AS_FLAGS) -o $@ $< --MD .deps/$(<:.s=.d)


$(DEPS) :
	@mkdir -p .deps


-include $(DEPS)/*.d
