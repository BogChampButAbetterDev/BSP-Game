#include "mapLoader.hpp"

namespace
{
    // Returns the index of `path` in the map's texture list, adding it if it's new.
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
            else if (word == "SECTOR")
            {
                parseState = ParseState::ExpectSectorData;
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

                // Paths contain digits, dots, underscores and spaces, which the
                // tokenizer would chop up, so read the rest of the line as raw text.
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
            else if (parseState == ParseState::ExpectSectorData)
            {
                if (!parseSector(value, out))
                {
                    return {};   // error already printed
                }
            }
            else if (parseState == ParseState::ExpectWallData)
            {
                if (!parseWalls(value, out, currentTexture))
                {
                    return {};   // error already printed
                }
            }

            i--;
        }
    }

    return out;
}

// SECTOR id floor ceiling light
// The id is a name you choose, and walls refer to it. The loader maps it to the
// sector's slot in Map::sectors, so ids can be any order or gaps in the file.
bool MapLoader::parseSector(float value, Map& out)
{
    m_pendingNumbers.push_back(value);
    if (m_pendingNumbers.size() < 4) return true;

    Sector s;
    s.id      = (int)m_pendingNumbers[0];
    s.floorHt   = m_pendingNumbers[1];
    s.ceilingHt = m_pendingNumbers[2];
    s.light   = m_pendingNumbers[3];

    if (out.sectorSlotById.count(s.id))
    {
        std::cerr << "Map parse failure: sector id " << s.id << " is defined twice\n";
        return false;
    }

    out.sectorSlotById[s.id] = (int)out.sectors.size();
    out.sectors.push_back(s);
    m_pendingNumbers.clear();
    return true;
}

// WALL x1 z1 x2 z2 frontSector backSector   (-1 = solid, no sector on that side)
// Returns false on a bad sector reference.
bool MapLoader::parseWalls(float value, Map& out, int texId)
{
    m_pendingNumbers.push_back(value);
    if (m_pendingNumbers.size() < 6) return true;

    Wall wall;

    wall.line.start = { m_pendingNumbers[0], m_pendingNumbers[1] }; // x1 z1
    wall.line.end   = { m_pendingNumbers[2], m_pendingNumbers[3] }; // x2 z2
    wall.texId       = texId;

    // The file holds sector ids. Walls store the slot in Map::sectors, so
    // drawing never needs a lookup.
    int frontId = (int)m_pendingNumbers[4];
    int backId  = (int)m_pendingNumbers[5];

    wall.frontSect = out.slotOfSector(frontId);
    wall.backSect  = (backId == -1) ? -1 : out.slotOfSector(backId);

    if (wall.frontSect < 0 || (backId != -1 && wall.backSect < 0))
    {
        std::cerr << "Map parse failure: wall " << out.walls.size()
                  << " uses sector id " << frontId << "/" << backId
                  << " which isn't defined above it\n";
        return false;
    }

    // Heights come from the front sector. Two-sided walls get their lower and
    // upper pieces from both sectors at draw time.
    wall.bottom = out.sectors[wall.frontSect].floorHt;
    wall.top    = out.sectors[wall.frontSect].ceilingHt;

    wall.computeNormal();

    // u runs from length/TEX_WORLD_W at the start down to 0 at the end, so the
    // texture isn't mirrored when you face the front of the wall.
    wall.UStart = wall.line.length() / TEX_WORLD_W;
    wall.UEnd   = 0.0f;

    out.walls.push_back(wall);
    m_pendingNumbers.clear();
    return true;
}
