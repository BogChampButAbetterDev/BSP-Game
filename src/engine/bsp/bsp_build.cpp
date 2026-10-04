#include "bsp_build.hpp"

namespace BSP
{
    int buildNode(std::vector<Wall> walls, std::vector<BSPNode>& nodes)
    {
        if (walls.empty()) return -1;

        Line spltr = walls[chooseSplitter(walls)].line;
        BSPNode node;
        node.splitter = spltr;
        std::vector<Wall> fronts, backs;
        
        for (const Wall& w : walls)
        {
            switch (classifyWall(spltr, w))
            {
                case Side::COPLANAR: node.coplanarWalls.push_back(w); break;
                case Side::FRONT: fronts.push_back(w); break;
                case Side::BACK: backs.push_back(w); break;
                case Side::SPANNING: 
                {
                    Wall f, b;
                    splitWall(w, spltr, f, b);
                    fronts.push_back(f);
                    backs.push_back(b);
                    break;
                }
            }
        }

        nodes.push_back(node);
        int idx = (int)nodes.size() - 1;
        int fIdx = buildNode(fronts, nodes);
        int bIdx = buildNode(backs, nodes);

        nodes[idx].front = fIdx;
        nodes[idx].back = bIdx;

        return idx;
    }
};

int chooseSplitter(const std::vector<Wall> &walls)
{
    int best = 0;
    int bestScore = INT_MAX;
    for (size_t i = 0; i < walls.size(); i++)
    {
        int f = 0; int b = 0; int s = 0;
        for (size_t j = 0; j < walls.size(); j++)
        {
            if (j == i) continue;
            switch (classifyWall(walls[i].line, walls[j]))
            {
                case Side::FRONT: f++; break;
                case Side::BACK: b++; break;
                case Side::SPANNING: s++; break;
                default: break;
            }
        }

        int score = 8 * s + std::abs(f - b);
        if (score < bestScore) { bestScore = score; best = i; }
    }

    return best;
}

BSPTree buildBSP(const std::vector<Wall> walls)
{
    BSPTree tree = {};
    tree.root = BSP::buildNode(walls, tree.nodes);
    return tree;
}
