#
#  Makefile of kernel modules
#
#  write by Forrest.zhang
#

obj-m += hello.o

obj-m += param.o

obj-m += gpl.o

obj-m += nogpl.o

obj-m += stack.o

obj-m += scull.o

obj-m += chardev.o

obj-m += netlink.o

obj-m += kthread.o

.PHONY : all clean test

all :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	rm -f test_open test_unicast

test:
	gcc -ggdb -Wstrict-prototypes -o test_open test_open.c
	gcc -ggdb -Wstrict-prototypes -o test_unicast test_unicast.c
