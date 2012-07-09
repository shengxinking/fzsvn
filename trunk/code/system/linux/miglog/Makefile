#
#	Makefile for miglog kernel module
#

obj-m += miglog.o

all:
	${MAKE} -C /lib/modules/$(shell uname -r)/build SUBDIRS=${PWD} modules;

clean: 
	${MAKE} -C /lib/modules/$(shell uname -r)/build SUBDIRS=${PWD} clean;

.PHONY: clean all


