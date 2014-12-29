# write by forrest
#

CXX = g++
CXXFLAGS = -Wall -ggdb

CXX_TARGETS = e1 e4
CXX_OBJS = e1.o e4.o
CXX_INCS =
CXX_LIBS =

all : $(CXX_TARGETS)

$(CXX_OBJS) : %.o : %.cpp
	$(CXX) -c $< $(CXXFLAGS) $(CXX_INCS)


$(CXX_TARGETS) : % : %.o
	$(CXX) -o $@ $< $(CXXFLAGS) $(CXX_LIBS)

.PHONY : all clean

clean :
	rm -rf $(CXX_OBJS) $(CXX_TARGETS) *~

