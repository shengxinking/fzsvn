# the rules for all cpppl program
# write by Forrest.zhang

CC     = gcc
CFLAGS = -ggdb -Wall

CXX      = g++
CXXFLAGS = -ggdb -Wall

ifdef CXX_TESTS
$(CXX_TESTS) : % : %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<
endif

ifdef CXX_PROGS
$(CXX_PROGS) : % : %.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(CXX_OBJS)
endif

ifdef CLEAN_FILES
.PHONY: clean
clean :
	rm -rf $(CLEAN_FILES)
endif