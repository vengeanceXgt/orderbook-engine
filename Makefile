CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Iinclude

all: build/test_orderbook build/bench

build:
	mkdir -p build

build/test_orderbook: build tests/test_orderbook.cpp src/orderbook.cpp include/orderbook.hpp
	$(CXX) $(CXXFLAGS) tests/test_orderbook.cpp src/orderbook.cpp -o $@

build/bench: build src/bench.cpp src/orderbook.cpp include/orderbook.hpp
	$(CXX) $(CXXFLAGS) -O3 src/bench.cpp src/orderbook.cpp -o $@

test: build/test_orderbook
	./build/test_orderbook

bench: build/bench
	./build/bench 2000000

clean:
	rm -rf build
.PHONY: all test bench clean
