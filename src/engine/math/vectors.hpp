#pragma once
#include <iostream>
#include <cmath>

struct Vector2
{
    float x = 0.0f;
    float y = 0.0f;

    Vector2 operator+(const Vector2 &other) const
    {
        return {x + other.x, y + other.y};
    }
 
    Vector2 &operator+=(const Vector2 &other)
    {
        x += other.x;
        y += other.y;
        return *this;
    }

    Vector2 operator-(const Vector2 &other) const { return {x - other.x, y - other.y}; }

    Vector2 &operator-=(const Vector2 &other)
    {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    Vector2 operator*(float scalar) const { return {x * scalar, y * scalar}; }

    Vector2 &operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    Vector2 operator/(float scalar) const { return {x / scalar, y / scalar}; }

    Vector2 &operator/=(float scalar)
    {
        x /= scalar;
        y /= scalar;
        return *this;
    }

    Vector2 operator-() const { return {-x, -y}; }

    float length() const { return std::sqrt((x * x) + (y * y)); }
    float len_squared() const { return (x * x) + (y * y); }
    Vector2 normalized() const
    {
        float len = length();
        if (len == 0.0f) return {0.0f, 0.0f};
        return {x / len, y / len}; 
    }

    float dot(const Vector2& other) const 
    {
        return (x * other.x) + (y * other.y);
    }

    float cross(const Vector2& other) const
    {
        return (x * other.y) - (y * other.x);
    }
};

struct Vector3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vector3 operator+(const Vector3 &other) const
    {
        return {x + other.x, y + other.y, z + other.z};
    }

    Vector3 &operator+=(const Vector3 &other)
    {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Vector3 operator-(const Vector3 &other) const { return {x - other.x, y - other.y, z - other.z}; }

    Vector3 &operator-=(const Vector3 &other)
    {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    Vector3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }

    Vector3 &operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    Vector3 operator/(float scalar) const { return {x / scalar, y / scalar, z / scalar}; }

    Vector3 &operator/=(float scalar)
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    Vector3 operator-() const { return {-x, -y, -z}; }

    float length() const { return std::sqrt((x * x) + (y * y) + (z * z)); }
    float len_squared() const { return (x * x) + (y * y) + (z * z); }
    Vector3 normalized() const
    {
        float len = length();
        if (len == 0.0f) return {0.0f, 0.0f, 0.0f};
        return {x / len, y / len, z / len}; 
    }

    float dot(const Vector3& other) const 
    {
        return (x * other.x) + (y * other.y) + (z * other.z);
    }

    Vector3 cross(const Vector3& other) const
    {
        return {
            (y * other.z) - (z * other.y),
            (z * other.x) - (x * other.z),
            (x * other.y) - (y * other.x)
        };
    }
};
