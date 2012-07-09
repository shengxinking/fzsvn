#
#  Makefile of kernel modules
#
#  write by Forrest.zhang
#

obj-m += connecttab.o


.PHONY : all clean test

all :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	rm -f test_open test_unicast ksync_utils

test:
	gcc -ggdb -Wstrict-prototypes -o test_open test_open.c
	gcc -ggdb -Wstrict-prototypes -o test_unicast test_unicast.c
	gcc -ggdb -Wstrict-prototypes -o ksync_utils ksync_utils.c
