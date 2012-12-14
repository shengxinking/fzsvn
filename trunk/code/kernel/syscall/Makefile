#
##	The Makefile for linux kernel 2.6
#


#	the sigle file modules
obj-m += syscall_replace.o
obj-m += syscall_replace1.o

#	the multi-files modules
obj-m += 
NNN-objs := 


.PHONY : all clean

all :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
	gcc -Wall -o syscall_replace_test syscall_replace_test.c
	gcc -Wall -o syscall_add_test syscall_add_test.c
	gcc -Wall -o syscall_80_test syscall_80_test.c
	gcc -Wall -o syscall_sysenter_test syscall_sysenter_test.c
	gcc -Wall -o syscall_glibc_test syscall_glibc_test.c

clean :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	rm syscall_add_test syscall_replace_test modules.* -f
	rm -f syscall_80_test syscall_sysenter_test syscall_glibc_test
