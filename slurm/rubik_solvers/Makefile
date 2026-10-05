# Build:  make            (all 6 programs)       make seq      make par
CXX      ?= g++
CXXFLAGS ?= -O3 -std=c++17 -march=native -Wall -Wextra -Wno-unknown-pragmas
SEQ = iddfs_seq bidir_seq astar_seq
PAR = iddfs_par bidir_par astar_par
HDR = $(wildcard common/*.hpp)

all: $(SEQ) $(PAR)
seq: $(SEQ)
par: $(PAR)

%_seq: %_seq.cpp $(HDR)
	$(CXX) $(CXXFLAGS) -o $@ $<
%_par: %_par.cpp $(HDR)
	$(CXX) $(CXXFLAGS) -fopenmp -o $@ $<

clean:
	rm -f $(SEQ) $(PAR)
.PHONY: all seq par clean
