#
#	Makefile for Bootloader
#

CC = gcc
AS = gas
LD = ld
OBJCOPY = objcopy
CFLAGS = -O2 -fno-builtin -nostdinc -I. -Wall -Wmissing-prototypes \
	 -Wunused -Wshadow -Wpointer-arith -falign-jumps=1 \
	 -falign-loops=1 -falign-functions=1 -Wundef -g


TARGET = first second bl_util

.PHONY : all clean distclean

all : $(TARGET)

first : first.S first.h
	$(CC) $(CFLAGS) -c first.S
	$(CC) -g -nostdlib -Wl,-N,-Ttext,7C00 -o first.exe first.o
	$(OBJCOPY) -O binary first.exe first

#stage1 : stage1.S stage1.h
#	$(CC) $(CFLAGS) -c stage1.S
#	$(CC) -g -nostdlib -Wl,-N,-Ttext,7C00 -o stage1.exe stage1.o
#	$(OBJCOPY) -O binary stage1.exe stage1

second : second.S
	$(CC) $(CFLAGS) -c second.S
	$(CC) -g -nostdlib -Wl,-N,-Ttext,8000 -o second.exe second.o
	$(OBJCOPY) -O binary second.exe second

bl_util : bl_util.c
	$(CC) -g -static -o bl_util bl_util.c

clean :
	rm -f *.o *.exe $(TARGET)

# here is copy from grub
#gcc -O2 -fno-builtin -nostdinc -DHAVE_CONFIG_H -I. -I. -I..  -Wall -Wmissing-prototypes -Wunused -Wshadow -Wpointer-arith -falign-jumps=1 -falign-loops=1 -falign-functions=1 -Wundef -g -c stage1.S
#gcc  -g  -nostdlib -Wl,-N,-Ttext,7C00 -o stage1.exec  stage1.o
#objcopy -O binary stage1.exec stage1
