#pragma once
#include <iostream>
#include <vector>
#include "bsp_build.hpp"

inline Wall makeWall(float x1, float z1, float x2, float z2)
{
    Wall w;
    w.line = {{x1, z1}, {x2, z2}};
    w.bottom = 0.0f;
    w.top = 3.0f;
    return w;
}

// The L-room with the pillar, in file order
inline std::vector<Wall> testRoomWalls()
{
    return
    {
        makeWall(-10,  0,  10,  0),   // 0
        makeWall( 10,  0,  10,  8),   // 1
        makeWall( 10,  8,  -2,  8),   // 2
        makeWall( -2,  8,  -2, 20),   // 3
        makeWall( -2, 20, -10, 20),   // 4
        makeWall(-10, 20, -10,  0),   // 5
        makeWall(  2,  3,   2,  5),   // 6 pillar
        makeWall(  2,  5,   4,  5),   // 7 pillar
        makeWall(  4,  5,   4,  3),   // 8 pillar
        makeWall(  4,  3,   2,  3),   // 9 pillar
    };
}

inline void report(const char* name, int got, int expected)
{
    std::cout << name << ": " << (got == expected ? "PASS" : "FAIL")
              << " (got " << got << ", expected " << expected << ")\n";
}

inline void testChooseSplitter()
{
    std::vector<Wall> walls = testRoomWalls();

    // Indices 0, 1, 4, 5 tie at 9, so the first one should win
    report("full room", chooseSplitter(walls), 0);

    // Swap the pillar wall (17) to the front: the winner should move to the next 9
    std::vector<Wall> reordered = walls;
    std::swap(reordered[0], reordered[6]);
    report("pillar first", chooseSplitter(reordered), 1);

    // Pillar wall splits the bottom wall (score 8); the bottom wall splits nothing (score 1)
    std::vector<Wall> pair = { walls[6], walls[0] };
    report("avoids split", chooseSplitter(pair), 1);

    // One wall: nothing to compare against
    std::vector<Wall> one = { walls[0] };
    report("single wall", chooseSplitter(one), 0);
}
