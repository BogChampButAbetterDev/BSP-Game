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

Vector3 Camera::camToWorldDir(Vector3 c) const
{
    float yaw = deg2rad(m_yaw), pitch = -deg2rad(m_pitch);
    float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(pitch), sp = sinf(pitch);
    float ry = c.y * cp + c.z * sp;
    float yz = -c.y * sp + c.z * cp;
    return { c.x * cy + yz * sy, ry, -c.x * sy + yz * cy };
}

Vector3 Camera::computeMove(float delta)
{
    float yawRad = deg2rad(m_yaw);
    m_forward = {sinf(yawRad), 0, cosf(yawRad)};
    m_right = {cosf(yawRad), 0, -sinf(yawRad)};

    return (m_forward * m_speed * delta) * m_direction.z + (m_right   * m_speed * delta) * m_direction.x;
}

void Camera::move(float delta)
{
    m_pos += computeMove(delta);
}
