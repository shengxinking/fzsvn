#
#	Makefile for Datastruct project
#

TOPDIR = ..

.PHONY : all clean distclean

TARGET = e1.3

all : $(TARGET)

e1.3 : e1.3.o
	$(CC) -o $@ $<



clean :
	rm -f *.o $(TARGET)

distclean:
	rm -rf *.o $(TARGET) .deps

include $(TOPDIR)/rules.Make
