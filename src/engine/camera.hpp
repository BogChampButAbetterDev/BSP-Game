#pragma once
#include <iostream>
#include <algorithm>

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

    Vector3 toCamSpace(Vector3 p) const;
    Vector2 projectCamSpace(Vector3 c) const;

    Vector2 projectPoint(Vector3 point) const { return projectCamSpace(toCamSpace(point)); }
    float viewDepth(Vector3 point) const { return toCamSpace(point).z; }

    void update(float delta);
    void move(float delta);

    Vector3 getPos() const { return m_pos; }
    void setPos(Vector3 pos) { m_pos = pos; m_pos.y += m_eyeHeight; }

    float getNear() const { return m_near; }
    float getYaw() const { return m_yaw; }

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
    float m_rotSpeed = 100.0f;

    float m_eyeHeight = 0.7f;

    int m_focalLen = HALF_WID;
    float m_near = 0.1;
};
