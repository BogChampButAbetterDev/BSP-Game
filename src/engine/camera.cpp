#include "camera.hpp"

Camera::Camera(Vector3 pos) 
 : m_pos(pos) 
{
    m_pos.y += m_eyeHeight;
}

Camera::~Camera()
{
}

Vector3 Camera::toCamSpace(Vector3 p) const
{
    float rx = p.x - m_pos.x, ry = p.y - m_pos.y, rz = p.z - m_pos.z;
    float yaw = deg2rad(m_yaw);
    float pitch = -deg2rad(m_pitch);
    float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(pitch), sp = sinf(pitch);
    float x  = rx * cy - rz * sy;
    float yz = rx * sy + rz * cy;
    return { x, ry * cp - yz * sp, ry * sp + yz * cp };
}

Vector2 Camera::projectCamSpace(Vector3 c) const
{
    return 
    {
        HALF_WID + (c.x / c.z) * m_focalLen,
        HALF_HT  - (c.y / c.z) * m_focalLen
    };
}

void Camera::update(float delta)
{
    m_direction = {0, 0, 0};

    if (in.w)      { m_direction.z = 1; }
    if (in.s)      { m_direction.z = -1; }
    if (in.a)      { m_direction.x = -1; }
    if (in.d)      { m_direction.x = 1; }
    if (in.arL)    { m_yaw -= m_rotSpeed * delta; }
    if (in.arR)    { m_yaw += m_rotSpeed * delta; }
    if (in.arUp)   { m_pitch -= m_rotSpeed * delta; }
    if (in.arDown) { m_pitch += m_rotSpeed * delta; }

    m_yaw += in.m_RX;
    m_pitch += in.m_RY;

    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

    in.m_RX = 0.0f;
    in.m_RY = 0.0f;

    m_direction = m_direction.normalized();
    move(delta);
} 

void Camera::move(float delta)
{
    float yawRad = deg2rad(m_yaw);
    m_forward = {sinf(yawRad), 0, cosf(yawRad)};
    m_right = {cosf(yawRad), 0, -sinf(yawRad)};

    m_pos += (m_forward * m_speed * delta) * m_direction.z;
    m_pos += (m_right * m_speed * delta) * m_direction.x;
}
