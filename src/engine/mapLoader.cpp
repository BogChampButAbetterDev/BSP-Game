#include "mapLoader.hpp"

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
                m_state = ParseState::ExpectMapName;
            }
            else if (word == "PLAYER")
            {
                m_state = ParseState::ExpectPlayerPos;
                m_pendingNumbers.clear();
            }
            else if (word == "WALL")
            {
                m_state = ParseState::ExpectWallData;
                m_pendingNumbers.clear();
            }
            else if (m_state == ParseState::ExpectMapName)
            {
                out.name = word;
                m_state = ParseState::None;
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

            float value = std::stof(buf.str());

            if (m_state == ParseState::ExpectPlayerPos)
            {
                m_pendingNumbers.push_back(value);
                if (m_pendingNumbers.size() == 3)
                {
                    out.playerPos = {m_pendingNumbers[0], m_pendingNumbers[1], m_pendingNumbers[2]};
                    m_state = ParseState::None;
                    m_pendingNumbers.clear();
                }
            }
            if (m_state == ParseState::ExpectWallData)
            {
                parseWalls(value, out);
            }

            i--;
        }
    }

    return out;
}

void MapLoader::parseWalls(float value, Map& out)
{
    m_pendingNumbers.push_back(value);
    if (m_pendingNumbers.size() == 6)
    {
        Wall wall;

        out.walls.push_back(wall);

        std::cout << "Wall " << out.walls.size() << ": ("
                  << wall.line.start.x << ", " << wall.line.start.y << ") -> ("
                  << wall.line.end.x << ", " << wall.line.end.y << ") "
                  << "bottom=" << wall.bottom << " top=" << wall.top << "\n";

        m_pendingNumbers.clear();
    }
}
