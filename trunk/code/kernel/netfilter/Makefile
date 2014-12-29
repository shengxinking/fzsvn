#
#  Makefile of kernel modules
#
#  write by Forrest.zhang
#

#obj-m += tcp_nat.o
obj-m += tproxy.o

.PHONY : all clean test

all :
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean
	rm -f tcp_nat_test test_open test_unicast ksync_utils

test:
	gcc -ggdb -Wstrict-prototypes -o test_open tcp_nat_test.c
