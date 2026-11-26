CXX = clang++
BASE_CXXFLAGS = -Wall -std=c++20 -MMD -MP -I./src -I/ucrt64/include/SDL3

# Debug build flags (default)
CXXFLAGS = $(BASE_CXXFLAGS) -g -O2

# Release build flags (no debug symbols, max optimization, defines NDEBUG)
RELEASE_CXXFLAGS = $(BASE_CXXFLAGS) -O3 -DNDEBUG

# Profile build flags
PROFILE_CXXFLAGS = $(BASE_CXXFLAGS) -g -gcodeview -O2
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

RELEASE_OBJ = $(SRC:src/%.cpp=obj_release/%.o)
RELEASE_DEP = $(RELEASE_OBJ:.o=.d)

DEMO_OBJ = $(SRC:src/%.cpp=obj_demo/%.o)
DEMO_DEP = $(DEMO_OBJ:.o=.d)

PROFILE_OBJ = $(SRC:src/%.cpp=obj_profile/%.o)
PROFILE_DEP = $(PROFILE_OBJ:.o=.d)

TARGET = bin/game.exe
RELEASE_DIR = dist/game_release
RELEASE_TARGET = $(RELEASE_DIR)/game.exe

DEMO_DIR = dist/game_demo
DEMO_TARGET = $(DEMO_DIR)/game_demo.exe

PROFILE_TARGET = bin/game_profile.exe

LIBS = -lSDL3 -lSDL3_image -lSDL3_ttf

# --- Default Debug Build ---
all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p bin
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(TARGET) $(LIBS) -mconsole

obj/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# --- Release Build (Packaged for Friends) ---
release: $(RELEASE_TARGET)
	cp -r assets $(RELEASE_DIR)/ 2>/dev/null || true
	ldd $(RELEASE_TARGET) | grep '/ucrt64/bin/' | awk '{print $$3}' | xargs -I {} cp {} $(RELEASE_DIR)/

$(RELEASE_TARGET): $(RELEASE_OBJ)
	mkdir -p $(RELEASE_DIR)
	$(CXX) $(RELEASE_CXXFLAGS) -s $(RELEASE_OBJ) -o $(RELEASE_TARGET) $(LIBS) -mwindows

obj_release/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(RELEASE_CXXFLAGS) -c $< -o $@

# --- Demo Build (Uses #ifdef DEMO_BUILD in C++) ---
demo: $(DEMO_TARGET)
	cp -r assets $(DEMO_DIR)/ 2>/dev/null || true
	ldd $(DEMO_TARGET) | grep '/ucrt64/bin/' | awk '{print $$3}' | xargs -I {} cp {} $(DEMO_DIR)/

$(DEMO_TARGET): $(DEMO_OBJ)
	mkdir -p $(DEMO_DIR)
	$(CXX) $(RELEASE_CXXFLAGS) -DDEMO_BUILD -s $(DEMO_OBJ) -o $(DEMO_TARGET) $(LIBS) -mwindows

obj_demo/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(RELEASE_CXXFLAGS) -DDEMO_BUILD -c $< -o $@

# --- Profile Build & Run ---
$(PROFILE_TARGET): $(PROFILE_OBJ)
	mkdir -p bin
	$(CXX) $(PROFILE_LDFLAGS) $(PROFILE_OBJ) -o $(PROFILE_TARGET) $(LIBS) -mconsole

obj_profile/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(PROFILE_CXXFLAGS) -c $< -o $@

profile: $(PROFILE_TARGET)
	samply record ./$(PROFILE_TARGET)

run: all
	./$(TARGET)

clean:
	rm -rf obj obj_release obj_demo obj_profile bin dist

-include $(DEP)
-include $(RELEASE_DEP)
-include $(DEMO_DEP)
-include $(PROFILE_DEP)
