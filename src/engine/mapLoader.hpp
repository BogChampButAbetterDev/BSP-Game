#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>

#include "math/geometry.hpp"
#include "math/vectors.hpp"

struct Map
{
    std::string name = "";
    Vector3 playerPos = {0, 0, 0};
    std::vector<Wall> walls;
};

enum class ParseState
{
    None,
    ExpectMapName,
    ExpectPlayerPos,
    ExpectWallData
};

class MapLoader
{
public:
    MapLoader() {}
    MapLoader(const std::string& filename);

    Map read();

private:
    std::string mapData = "";

    ParseState m_state = ParseState::None;
    std::vector<float> m_pendingNumbers;

    void parseWalls(float value, Map& out);
};
