CXX = g++

CXXFLAGS = -Wall -g -O0 -std=c++17 -MMD -MP -I./src -I/ucrt64/include/SDL3

DEP = $(OBJ:.o=.d)

SRC = \
	src/main.cpp \
	$(wildcard src/engine/*.cpp) \
	$(wildcard src/renderer/*.cpp) \
	$(wildcard src/game/*.cpp) \
	$(wildcard src/engine/math/*.cpp) \
	$(wildcard src/globals/*.cpp) \
	$(wildcard src/third_party/*.cpp)

OBJ = $(SRC:src/%.cpp=obj/%.o)

TARGET = bin/game.exe

LIBS = -lSDL3 -lSDL3_image -lSDL3_ttf

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) -static-libasan $(OBJ) -o $(TARGET) $(LIBS) -mconsole

obj/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

-include $(DEP)
	
run: all
	./$(TARGET)

clean:
	rm -rf obj bin
