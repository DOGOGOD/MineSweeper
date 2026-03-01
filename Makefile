TARGET := minesweeper
SRCS := main.cpp $(wildcard src/*.cpp)
CXX := g++
CXXFLAGS := -Wall -std=c++14 -Iinclude
LDLIBS :=

ifeq ($(OS),Windows_NT)
LDLIBS += -lwinmm
endif

run: $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS) $(LDLIBS)
	./$(TARGET)
