CXX = g++
CXXFLAGS = -Wall -std=c++17
CORE = main.cpp $(wildcard src/*.cpp)
LIBS = $(wildcard src/libraries/*.cpp)
SOURCES = $(CORE) $(LIBS)
HEADERS = $(wildcard include/*.h) $(wildcard include/libraries/*.h)

bp: $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o bp

clean:
	rm -f bp

.PHONY: clean
