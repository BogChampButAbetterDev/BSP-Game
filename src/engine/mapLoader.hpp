#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>

#include "camera.hpp"
#include "math/geometry.hpp"
#include "math/vectors.hpp"

#include "bsp/bsp.hpp"

struct Map
{
    std::string name = "";
    Vector3 playerPos = {0, 0, 0};
    std::vector<Wall> walls;

    BSPTree tree;

    void onLoad(Camera& cam)
    {
        cam.setPos(playerPos);
    }
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

    std::vector<float> m_pendingNumbers;

    void parseWalls(float value, Map& out);
};
