CXX = g++
CXXFLAGS = -Wall -g -O2 -std=c++20 -MMD -MP -I./src -I/ucrt64/include/SDL3

PROFILE_CXX = clang++
PROFILE_CXXFLAGS = -Wall -g -gcodeview -O2 -std=c++20 -MMD -MP -I./src -I/ucrt64/include/SDL3
PROFILE_LDFLAGS = -fuse-ld=lld -Wl,--pdb=

SRC = \
	src/main.cpp \
	$(wildcard src/engine/*.cpp) \
	$(wildcard src/engine/bsp/*.cpp) \
	$(wildcard src/renderer/*.cpp) \
	$(wildcard src/game/*.cpp) \
	$(wildcard src/engine/math/*.cpp) \
	$(wildcard src/globals/*.cpp) \
	$(wildcard src/third_party/*.cpp)

OBJ = $(SRC:src/%.cpp=obj/%.o)
DEP = $(OBJ:.o=.d)

PROFILE_OBJ = $(SRC:src/%.cpp=obj_profile/%.o)
PROFILE_DEP = $(PROFILE_OBJ:.o=.d)

TARGET = bin/game.exe
PROFILE_TARGET = bin/game_profile.exe

LIBS = -lSDL3 -lSDL3_image -lSDL3_ttf

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) -static-libasan $(OBJ) -o $(TARGET) $(LIBS) -mconsole

obj/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- Profile Build & Run ---
$(PROFILE_TARGET): $(PROFILE_OBJ)
	mkdir -p bin
	$(PROFILE_CXX) $(PROFILE_LDFLAGS) $(PROFILE_OBJ) -o $(PROFILE_TARGET) $(LIBS) -mconsole

obj_profile/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(PROFILE_CXX) $(PROFILE_CXXFLAGS) -c $< -o $@

profile: $(PROFILE_TARGET)
	samply record ./$(PROFILE_TARGET)

run: all
	./$(TARGET)

clean:
	rm -rf obj obj_profile bin

-include $(DEP)
-include $(PROFILE_DEP)
