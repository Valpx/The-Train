#pragma once

#include <functional>

struct Coord
{
    int x, y;

    bool operator==(const Coord &other) const
    {
        return x == other.x && y == other.y;
    }
};

struct CoordHash
{
    std::size_t operator()(const Coord &c) const
    {
        return std::hash<int>()(c.x) ^ (std::hash<int>()(c.y) << 1);
    }
};
