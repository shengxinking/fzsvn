#
#  Makefile of linux 2.6 kernel module
#
#  write by Forrest.zhang
#

PWD = $(shell pwd)
SRC = /lib/modules/$(shell uname -r)/build

obj-m += scull.o
#obj-m += mod_scull.o

.PHONY : all clean

all:
	make -C $(SRC) SUBDIRS=$(PWD) modules

clean:
	make -C $(SRC) SUBDIRS=$(PWD) clean

