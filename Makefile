
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -Werror -pedantic -O3 -Iinclude

# All object files required by the project
OBJS = build/key_encoder.o build/db.o build/wal.o build/memtable.o

.PHONY: all clean

all: build/tsdb_main

# Create build directory if it does not exist
build:
	mkdir -p build

# Generic rule to compile any src/*.cpp into build/*.o
build/%.o: src/%.cpp | build
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule to link object files with main.cpp into the final executable
build/tsdb_main: $(OBJS) main.cpp | build
	$(CXX) $(CXXFLAGS) $(OBJS) main.cpp -o $@

clean:
	rm -rf build tsdb.wal test_output.bin