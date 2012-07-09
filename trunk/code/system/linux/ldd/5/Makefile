#
#  Makefile of kernel 2.6 modules
#

version = $(shell uname -r)
kernel_dir = /lib/modules/$(version)/build
current_dir = $(shell pwd)

obj-m += atomic.o

obj-m += rwsemaphore.o

obj-m += semaphore.o

.PHONY: all clean

all:
	make -C $(kernel_dir) M=$(current_dir) modules

clean:
	make -C $(kernel_dir) M=$(current_dir) clean

