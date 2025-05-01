#pragma once

#include <functional>

struct Vector2D
{
    int x, y;

    Vector2D() : x{0}, y{0} {}
    Vector2D(const std::vector<int> &coords) : x{coords[0]}, y{coords[1]} {}
    Vector2D(int a, int b) : x{a}, y{b} {}

    bool operator==(const Vector2D &other) const
    {
        return x == other.x && y == other.y;
    }

    Vector2D operator+(const Vector2D &other) const
    {
        return {x + other.x, y + other.y};
    }

    Vector2D operator-(const Vector2D &other) const
    {
        return {x - other.x, y - other.y};
    }

    int manhattanDistance(const Vector2D &other) const
    {
        return std::abs(x - other.x) + std::abs(y - other.y);
    }

    bool isNeighbor(const Vector2D &other) const
    {
        return manhattanDistance(other) == 1;
    }
};

struct Vector2DHash
{
    std::size_t operator()(const Vector2D &c) const
    {
        return std::hash<int>()(c.x) ^ (std::hash<int>()(c.y) << 1);
    }
};
