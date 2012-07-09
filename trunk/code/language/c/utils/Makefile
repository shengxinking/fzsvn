#
#  Makefile of utils
#

CC = gcc
CFLAGS = -ggdb -Wstrict-prototypes -Wall -static
LDFLAGS =

ifneq (".deps", "$(wildcard .deps)")
	DEPS = .deps
endif


TARGETS = fwrite getline

.PHONE : all clean distclean

all : $(TARGETS)


fwrite : fwrite.o
	$(CC) -o $@ $^ $(LDFLAGS)

getline : getline.o
	$(CC) -o $@ $^ $(LDFLAGS)


clean:
	rm -f *.o $(TARGETS)


distclean:
	rm -rf *.o $(TARGETS) .deps



%.o : %.c $(DEPS)
	$(CC) -o $@ -c $< $(CFLAGS) -MD -MF $(DEPS)/$(<:.s=.d)


$(DEPS) :
	@ [ -d $@ ] || mkdir -p $@


-include $(DEPS)/*.d

