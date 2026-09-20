#pragma once
#include <iostream>
#include <memory>

#include "engine/input.hpp"
#include "math/vectors.hpp"

#include "globals/globals.hpp"
#include "globals/math_utils.hpp"

class Camera
{
public:
    Camera() {}
    explicit Camera(Vector3 pos);

    ~Camera();

    Vector2 projectPoint(Vector3 point);
    float viewDepth(Vector3 point);

    void update(float delta);
    void move(float delta);

    Vector3 getPos() { return m_pos; }

    Input in;

private:
    Vector3 m_pos;
    Vector3 m_direction;
    Vector3 m_up = {0, 1, 0};
    Vector3 m_forward = {0, 0, 1};
    Vector3 m_right;

    float m_pitch = 0.0f;
    float m_yaw = 0.0f;

    int m_speed = 10;
    float m_rotSpeed = 0.8f;

    int m_focalLen = HALF_WID;
    float m_near = 0.01;
};
