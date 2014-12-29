#
#  write by Forrest.zhang
#

CXX      = g++
CXXFLAGS = -Wall -ggdb


CXX_PROGS = SatelliteTest WindowTest AccessTest
CXX_OBJS  = Satellite.o Window.o Access.o
CXX_LIBS  =
CXX_INCS  =


all : $(CXX_OBJS) $(CXX_PROGS)


$(CXX_OBJS) : %.o : %.cpp %.hpp
	$(CXX) $(CXXFLAGS) -c $< $(CXX_INCS)


$(CXX_PROGS) : % : %.cpp $(CXX_OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(CXX_INCS) $(CXX_LIBS)


.PHONY : all clean


clean:
	rm -f $(CXX_OBJS) $(CXX_PROGS)










