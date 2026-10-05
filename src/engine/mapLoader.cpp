#include "mapLoader.hpp"

namespace
{
    int textureIndex(Map& map, const std::string& path)
    {
        for (size_t i = 0; i < map.texturePaths.size(); i++)
        {
            if (map.texturePaths[i] == path) return (int)i;
        }

        map.texturePaths.push_back(path);
        return (int)map.texturePaths.size() - 1;
    }
}

MapLoader::MapLoader(const std::string &filename)
{
    std::ifstream file(filename);
    if (file.is_open())
    {
        std::stringstream buf;

        buf << file.rdbuf();

        mapData = buf.str();

        file.close();
    }
    else 
    {
        std::cerr << "Couldn't open " << filename << ".\n";
        std::cout << "Map will not load\n";
        mapData = "";
    }
}

Map MapLoader::read()
{
    if (mapData.length() <= 0) return {};
    ParseState parseState = ParseState::None;
    int currentTexture = -1;   // -1 = no TEXTURE line seen yet (flat color)

    Map out;

    for (size_t i = 0; i < mapData.length(); i++)
    {
        unsigned char currentChar = (unsigned char)mapData[i];
        
        if (std::isalpha(currentChar))
        {
            std::stringstream buf;

            while (i < mapData.length() && std::isalpha((unsigned char)mapData[i]))
            {
                buf << mapData[i];
                i++;
            }

            std::string word = buf.str();

            if (word == "MAP")
            {
                parseState = ParseState::ExpectMapName;
            }
            else if (word == "PLAYER")
            {
                parseState = ParseState::ExpectPlayerPos;
                m_pendingNumbers.clear();
            }
            else if (word == "WALL")
            {
                parseState = ParseState::ExpectWallData;
                m_pendingNumbers.clear();
            }
            else if (word == "TEXTURE")
            {
                parseState = ParseState::None;
                m_pendingNumbers.clear();

                while (i < mapData.length() && (mapData[i] == ' ' || mapData[i] == '\t')) { i++; }

                size_t pathStart = i;
                while (i < mapData.length() && mapData[i] != '\n' && mapData[i] != '\r') { i++; }

                std::string path = mapData.substr(pathStart, i - pathStart);
                while (!path.empty() && (path.back() == ' ' || path.back() == '\t')) { path.pop_back(); }

                if (path.empty())
                {
                    std::cerr << "Map parse failure: TEXTURE with no path\n";
                    return {};
                }

                currentTexture = textureIndex(out, path);
            }
            else if (parseState == ParseState::ExpectMapName)
            {
                out.name = word;
                parseState = ParseState::None;
            }
            else 
            {
                std::cout << "Unknown keyword: " << word << "\n";
            }

            i--;
        }
        else if (std::isdigit((unsigned char)mapData[i]) || mapData[i] == '-')
        {
            std::stringstream buf;

            if (mapData[i] == '-') { buf << mapData[i]; i++; }

            while (i < mapData.length() && (std::isdigit((unsigned char)mapData[i]) || mapData[i] == '.'))
            {
                buf << mapData[i];
                i++;
            }

            float value = 0.0f;
            try
            {
                value = std::stof(buf.str());
            }
            catch(const std::exception&)
            {
                std::cerr << "Map parse failure due to bad number '" << buf.str() << "' at character '" << i << "\n";
                return {};
            }

            if (parseState == ParseState::ExpectPlayerPos)
            {
                m_pendingNumbers.push_back(value);
                if (m_pendingNumbers.size() == 3)
                {
                    out.playerPos = {m_pendingNumbers[0], m_pendingNumbers[1], m_pendingNumbers[2]};
                    parseState = ParseState::None;
                    m_pendingNumbers.clear();
                }
            }
            if (parseState == ParseState::ExpectWallData)
            {
                parseWalls(value, out, currentTexture);
            }

            i--;
        }
    }

    return out;
}

void MapLoader::parseWalls(float value, Map& out, int texId)
{
    m_pendingNumbers.push_back(value);
    if (m_pendingNumbers.size() == 6)
    {
        Wall wall;

        wall.line.start = { m_pendingNumbers[0], m_pendingNumbers[1] }; // x1 y1
        wall.line.end   = { m_pendingNumbers[2], m_pendingNumbers[3] }; // x2 y2
        wall.bottom     = m_pendingNumbers[4];                          // bottom
        wall.top        = m_pendingNumbers[5];                          // top
        wall.texId      = texId;                                        // index into Map::texturePaths, -1 = none

        wall.computeNormal();
        wall.UStart = wall.line.length() / TEX_WORLD_H;
        wall.UEnd = 0.0f;

        out.walls.push_back(wall);
        m_pendingNumbers.clear();
    }
}
