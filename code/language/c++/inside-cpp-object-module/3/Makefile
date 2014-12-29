# the make file of chapter 3


CXX = g++
CXXFLAGS = -ggdb -Wall

CXXTARGETS = e1
CXXOBJS = e1.o
CXXINCS =
CXXLIBS =

all: $(CXXTARGETS)

$(CXXOBJS) : %.o : %.cpp
	$(CXX) -c $< $(CXXFLAGS) $(CXXINCS)

$(CXXTARGETS) : % : %.o
	$(CXX) -o $@ $< $(CXXFLAGS) $(CXXLIBS)


.PHONY: all clean

clean:
	rm -rf $(CXXOBJS) $(CXXTARGETS)
