
OMP_FLAGS = -fopenmp

DEBUG_FLAGS   = -g
RELEASE_FLAGS = -g -DNDEBUG -O3 -march=native -mtune=native

CFG_FLAGS = $(RELEASE_FLAGS)

STD_FLAGS     = -std=c++1z
WALL_FLAGS    = -Wall -Wextra -Werror

CXXFLAGS = $(CFG_FLAGS) $(OPTIMIZATION_FLAGS) $(STD_FLAGS) $(OMP_FLAGS) $(WALL_FLAGS)
CXXLD    = $(CXX)

all: bin/bitcounter

bin/bitcounter: Makefile $(wildcard src/*)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) -c src/bitcnt.cpp   -o bin/bitcnt.o
	$(CXX) $(CXXFLAGS) -c src/main.cpp     -o bin/main.o
	$(CXX) $(CXXFLAGS) -c src/sys-unix.cpp -o bin/sys.o
	$(CXXLD) $(CXXFLAGS) bin/*.o -o bin/bitcounter

asm: Makefile $(wildcard src/*)
		$(CXX) $(CXXFLAGS) -S -fverbose-asm src/bitcnt.cpp -o -

clean:
	-rm -r bin