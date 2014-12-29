#
## the Makefile of linux 2.6 kernel modules
#


obj-m += hello_1.o

obj-m += hello_2.o

obj-m += hello_3.o

obj-m += hello_4.o

obj-m += hello_5.o

obj-m += miglog.o

obj-m += chardev.o

obj-m += ioctl.o

obj-m += syscall.o

obj-m += sleep.o

obj-m += procfs_1.o

obj-m += procfs_2.o

obj-m += procfs_3.o


.PHONY : all clean test

all: test
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean


test:
	gcc -ggdb -o test_read test_read.c
	gcc -ggdb -o test_ioctl test_ioctl.c


