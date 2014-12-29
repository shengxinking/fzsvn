
CXX = g++
CXXFLAGS = -Wall -ggdb

CXX_TESTS = t1 t2 t3 t4 t5 t6 t7 t8 t9 t10 t11 t12

.PHONY : all
all : $(CXX_TESTS)

$(CXX_TESTS) : % : %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

.PHONY : clean
clean:
	rm -rf $(CXX_TESTS)

