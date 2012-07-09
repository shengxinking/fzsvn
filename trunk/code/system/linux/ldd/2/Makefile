#
#  Makefile of linux 2.6 kernel module
#
#  write by Forrest.zhang
#

PWD = $(shell pwd)
SRC = /lib/modules/$(shell uname -r)/build

obj-m += hello.o
obj-m += param.o

.PHONY : all clean

all:
	make -C $(SRC) M=$(PWD) modules

clean:
	make -C $(SRC) M=$(PWD) clean

