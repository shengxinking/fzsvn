#
##	The Makefile for linux kernel 2.6
#


#	the sigle file modules
obj-m += af_fz.o


#	the multi-files modules
#obj-m += NNN.o
#NNN-objs := 

.PHONY : all clean

all :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean


