#
#  make a route tools using netlink
#
#  write by Forrest.zhang
#

CC = gcc
CFLAGS = -ggdb -Wall -Wstrict-prototypes

TARGET = route
SRCS   = $(wildcard *.c)
OBJS   = $(patsubst %.c,%.o,$(SRCS))

ifneq (.deps, $(wildcard .deps))
    DEPS = .deps
endif

all : $(TARGET)

$(TARGET) : $(OBJS)
	echo "link $@ ..."; \
	$(CC) -o $@ $^ $(CFLAGS)

$(OBJS) : %.o : %.c $(DEPS)
	@echo "compile $< ..."; \
	$(CC) $(CFLAGS) -c -o $@ $< ; \
	$(CC) -c $< -MM -MF $(patsubst %.o,%.d,$@); \
	mv $(patsubst %.o,%.d,$@) .deps

$(DEPS) :
	@if [ ! -d .deps ]; then \
            mkdir .deps; \
        fi

.PHONY: clean

clean:
	@echo "clean ..." ; \
	rm -rf .deps *.o $(TARGET)

-include .deps/*.d

