#pragma once
#include <iostream>
#include <vector>

#include "bsp.hpp"
#include "engine/math/geometry.hpp"

int chooseSplitter(const std::vector<Wall>& walls);

BSPTree buildBSP(const std::vector<Wall> walls);
