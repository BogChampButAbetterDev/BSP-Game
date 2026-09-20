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
    float pitchRad = -deg2rad(m_pitch);

    float rotatedX = relX * cosf(yawRad) - relZ * sinf(yawRad);
    float yawZ = relX * sinf(yawRad) + relZ * cosf(yawRad);

    float rotatedY = relY * cosf(pitchRad) - yawZ * sinf(pitchRad);
    float rotatedZ = relY * sinf(pitchRad) + yawZ * cosf(pitchRad);

    float scrX = HALF_WID + ((rotatedX / rotatedZ) * m_focalLen);
    float scrY = HALF_HT - ((rotatedY / rotatedZ) * m_focalLen);

    return {scrX, scrY};
}

float Camera::viewDepth(Vector3 point)
{
    float relX = point.x - m_pos.x;
    float relY = point.y - m_pos.y;
    float relZ = point.z - m_pos.z;

    float yawRad = deg2rad(m_yaw);
    float pitchRad = -deg2rad(m_pitch);

    float yawZ = relX * sinf(yawRad) + relZ * cosf(yawRad);
    
    float rotatedZ = relY * sinf(pitchRad) + yawZ * cosf(pitchRad);

    return rotatedZ;
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
