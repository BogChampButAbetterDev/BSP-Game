#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <string>
#include <unordered_map>

#include "camera.hpp"
#include "math/geometry.hpp"
#include "math/vectors.hpp"

#include "bsp/bsp.hpp"

struct Map
{
    std::string name = "";
    Vector3 playerPos = {0, 0, 0};
    std::vector<Wall> walls;

    std::vector<Sector> sectors;

    std::unordered_map<int, int> sectorSlotById;

    int slotOfSector(int id) const
    {
        auto it = sectorSlotById.find(id);
        return it == sectorSlotById.end() ? -1 : it->second;
    }

    std::vector<std::string> texturePaths;

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
    ExpectSectorData,
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

    bool parseSector(float value, Map& out);
    bool parseWalls(float value, Map& out, int texId);
};
