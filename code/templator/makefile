#
##	The Makefile for linux kernel 2.6
#

# global definitions.


#	the sigle file modules
obj-m += 


#	the multi-files modules
obj-m += NNN.o
NNN-objs := 

# the library file
lib-y += NNN.o


.PHONY : all clean

all :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean


