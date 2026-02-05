TARGET := minesweeper
SRCS := main.cpp $(wildcard src/*.cpp)
CXX := g++
CXXFLAGS := -Wall -std=c++14 -Iinclude

run: $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)
	./$(TARGET)
