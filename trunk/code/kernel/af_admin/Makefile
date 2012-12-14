#
#  The Makefile for linux kernel 2.6
#

#  the sigle file modules
obj-m += af_admin.o

.PHONY : all clean

all :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	gcc -ggdb -Wall -o af_admin_cli af_admin_cli.c
	gcc -ggdb -Wall -o af_admin_svr af_admin_svr.c

clean :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	rm -f af_admin_test

