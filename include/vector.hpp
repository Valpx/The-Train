#pragma once

#include <functional>

struct Vector2D
{
    int x, y;

    bool operator==(const Vector2D &other) const
    {
        return x == other.x && y == other.y;
    }
};

struct Vector2DHash
{
    std::size_t operator()(const Vector2D &c) const
    {
        return std::hash<int>()(c.x) ^ (std::hash<int>()(c.y) << 1);
    }
};
