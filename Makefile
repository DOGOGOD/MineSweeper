SRCS := $(wildcard *.cpp) $(wildcard */*.cpp)

run: $(SRCS)
	g++ -Wall -std=c++14 -o minesweeper $(SRCS)
	./minesweeper
