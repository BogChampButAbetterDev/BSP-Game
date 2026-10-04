#pragma once
#include <iostream>
#include <vector>

#include "engine/math/geometry.hpp"

struct BSPNode
{
    Line splitter;
    std::vector<Wall> coplanarWalls;
    int front = -1;
    int back = -1;
}; 

struct BSPTree
{
    std::vector<BSPNode> nodes;
    int root = -1;

    bool empty() const { return root == -1; }
    BSPNode& nodeAt(int i) { return nodes[i]; }
};
