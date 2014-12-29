#
#
#


CXX = g++
CXXFLAGS = -Wall -ggdb

CXX_PROGS = t1 t2 t3 t4 t5 t6 t7 t8 t9 t10 t11
CXX_OBJS = t1.o t2.o t3.o t4.o t5.o t6.o t7.o t8.o t9.o t10.o t11.o
CXX_LIBS =
CXX_INCS =

all : $(CXX_PROGS)

$(CXX_PROGS) : % : %.o
	$(CXX) -o $@ $< $(CXXLIBS)

$(CXX_OBJS) : %.o : %.cpp
	$(CXX) -c $< $< $(CXXFLAGS) $(CXX_INCS) $(CXX_LIBS)


.PHONY: all clean

clean:
	rm -f $(CXX_OBJS) $(CXX_PROGS)

