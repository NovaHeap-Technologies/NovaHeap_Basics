CXX ?= g++
CXXFLAGS = -std=c++11 -Wall -Wextra -Werror -I. -I../../src
SOURCES = test_basics.cpp ../../src/NovaHeap_Basics.cpp ../../src/NHCommands.cpp

test: test_basics
	./test_basics

test_basics: $(SOURCES) ../../src/NovaHeap_Basics.h Arduino.h
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES)

clean:
	rm -f test_basics test_basics.exe

.PHONY: test clean
