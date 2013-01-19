#
#	the rules file of all APUE program
#

CC = gcc
CFLAGS = -ggdb -Wall -Wstrict-prototypes

ifdef TARGET_1
$(TARGET_1) : $(OBJS_1) $(LIBS_1)
	$(CC) -o $(TARGET_1) $(OBJS_1) $(LIBS_1)
endif

%.o : %.c
	$(CC) -o $@ -c $< $(CFLAGS)

ifdef CLEAN
.PHONY : clean
clean :
	rm -rf $(CLEAN)
endif
