# this is the Makefiel for gnu Make

CC     = gcc
CFLAGS = -ggdb -Wall -Wstrict-prototypes

TARGET  = netdevice
OBJS    = netdevice.o netdevice_test.o

.PHONY : all clean

all: $(TARGET) $(TARGET1) $(TARGET2) $(TARGET3)

$(TARGET) : $(LIBS) $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^
	

clean :
	rm -rf $(TARGET) $(OBJS) 
	
