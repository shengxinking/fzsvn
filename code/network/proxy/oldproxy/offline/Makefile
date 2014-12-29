TOPDIR=..

OBJS = spp_frag3.o spp_stream4.o   util.o plugbase.o  \
	perf.c  decode.o  sfhashfcn.o\
	frag.o  cap.o mempool.o stream_api.o


TARGET_A = libfrag.a
TARGET_A_OBJECTS = $(OBJS) 

#UPGRADE_TARGET_PROG = smtp
UPGRADE_TARGET_PROG_OBJECTS = frag.o

LIBS = $(CFGLIB) 

UPGRADE_TARGET_PROG_LIBS = \
    -lpcap \
    -lm \
    $(CORELIBS)

EXTRA_SYSLIBS= -ldl $(DEVDIR)/lib/librt.a  -lcrypt -lpcap -lm

EXTRA_CFLAS += -D_LVM_H_INCLUDE

TARGET_PROG = frag 
OBJECTS_PROG =  $(OBJS)

all: $(TARGET_PROG)

clean:
	-rm -f core *.a *.o *.so *~ extract $(TARGET_PROG) $(TARGET_SO) libfrag.a
include $(TOPDIR)/rules.Make
