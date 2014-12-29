#
# the makefile of cpppl
#


CXX = g++
CXXFLAGS = -Wall -ggdb

CXX_PROGS = DateTest
CXX_OBJS = Date.o TreeNode.o
CXX_LIBS =
CXX_INCS =

all : $(CXX_PROGS) $(CXX_OBJS)

$(CXX_OBJS) : %.o : %.cpp %.hpp
	$(CXX) -c $< $(CXXFLAGS) $(CXX_INCS) $(CXX_LIBS)

$(CXX_PROGS) : % : %.cpp $(CXX_OBJS)
	$(CXX) -o $@ $< $(CXX_OBJS) $(CXXFLAGS) $(CXX_INCS) $(CXX_LIBS)

.PHONY: all clean

clean:
	rm -f $(CXX_OBJS) $(CXX_PROGS)

