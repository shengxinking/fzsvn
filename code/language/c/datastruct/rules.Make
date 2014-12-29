#
#       the Main makefile for Datastruct project
#

CC = gcc
CFLAGS = -ggdb -Wall -Wstrict-prototypes

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif


%.o : %.c $(DEPS)
	$(CC) -o $@ -c $< $(CFLAGS) -MD -MF .deps/$(<:.s=.d)


$(DEPS) :
	@ [ -d $@ ] || mkdir -p $@


-include $(DEPDIR)/*.d


