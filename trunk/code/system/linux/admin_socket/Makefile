#
#	Makefile for admin socket kernel module
#

obj-m += admin_socket.o

.PHONY: clean all


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean: 
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean



