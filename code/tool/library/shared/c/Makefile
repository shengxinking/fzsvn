#
#	the Makefile of test static library
#

CC = gcc
CFLAGS = -ggdb -Wall

CXX = g++
CXXFLAGS = -ggdb -Wall

AR = ar -rcs

LIB_A = libtest.a
LIB_A_OBJS = test.o

PROG = main
PROG_OBJS = main.o
PROG_LIBS = libtest.a

all : $(LIB_A) $(PROG)

$(LIB_A) : $(LIB_A_OBJS)
	$(AR) $@ $<

$(PROG) : $(PROG_OBJS)
	$(CC) -o $@ $^ $(PROG_LIBS)

%.o : %.c
	$(CC) -o $@ -c $< $(CFLAGS)

%.o : %.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

clean :
	$(RM) $(LIB_A) $(LIB_A_OBJS) $(PROG) $(PROG_OBJS)


