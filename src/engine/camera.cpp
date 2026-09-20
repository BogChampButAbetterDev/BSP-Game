#include "camera.hpp"

Camera::Camera(Vector3 pos) 
 : m_pos(pos) 
{
}

Camera::~Camera()
{
}

Vector2 Camera::projectPoint(Vector3 point)
{
    float relX = point.x - m_pos.x;
    float relY = point.y - m_pos.y;
    float relZ = point.z - m_pos.z;

    float yawRad = deg2rad(m_yaw);
    float rotatedX = relX * cosf(yawRad) - relZ * sinf(yawRad);
    float rotatedZ = relX * sinf(yawRad) + relZ * cosf(yawRad);

    float scrX = HALF_WID + ((rotatedX / rotatedZ) * m_focalLen);
    float scrY = HALF_HT - ((relY / rotatedZ) * m_focalLen);

    return {scrX, scrY};
}

float Camera::viewDepth(Vector3 point)
{
    float relX = point.x - m_pos.x;
    float relZ = point.z - m_pos.z;

    return relX * sinf(deg2rad(m_yaw)) + relZ * cosf(deg2rad(m_yaw));
}

void Camera::update(float delta)
{
    m_direction = {0, 0, 0};

    if (in.w)   { m_direction.z = 1; }
    if (in.s)   { m_direction.z = -1; }
    if (in.a)   { m_direction.x = -1; }
    if (in.d)   { m_direction.x = 1; }
    if (in.arL) { m_yaw -= m_rotSpeed; }
    if (in.arR) { m_yaw += m_rotSpeed; }

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
