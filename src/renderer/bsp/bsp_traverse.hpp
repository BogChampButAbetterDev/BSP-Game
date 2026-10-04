#pragma once
#include <iostream>
#include <vector>

#include "engine/bsp/bsp.hpp"

inline void BSPTraverse(const BSPTree& tree, int nodeIDX, Vector2 posXZ, std::vector<Wall>& order)
{
    if (nodeIDX == -1) return;

    const BSPNode& node = tree.nodes[nodeIDX];

    int far = 0;
    int near = 0;
    if (node.splitter.sideOf(posXZ) > 0)
    {
        near = node.front;
        far = node.back;
    }  
    else
    {
        near = node.back;
        far = node.front;
    }

    BSPTraverse(tree, far, posXZ, order);
    for (const Wall& w : node.coplanarWalls)
    {
        order.push_back(w);
    }
    BSPTraverse(tree, near, posXZ, order);
}
