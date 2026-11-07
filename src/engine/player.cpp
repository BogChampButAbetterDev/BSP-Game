#include "player.hpp"

Player::Player(Vector3 pos)
{
    m_cam = std::make_unique<Camera>(pos);
}

void Player::update(float delta)
{
    Camera* camPtr = m_cam.get();
    camPtr->m_direction = {0, 0, 0};

    if (camPtr->in.w)      { camPtr->m_direction.z = 1; }
    if (camPtr->in.s)      { camPtr->m_direction.z = -1; }
    if (camPtr->in.a)      { camPtr->m_direction.x = -1; }
    if (camPtr->in.d)      { camPtr->m_direction.x = 1; }
    if (camPtr->in.arL)    { camPtr->m_yaw -= camPtr->m_rotSpeed * delta; }
    if (camPtr->in.arR)    { camPtr->m_yaw += camPtr->m_rotSpeed * delta; }
    if (camPtr->in.arUp)   { camPtr->m_pitch -= camPtr->m_rotSpeed * delta; }
    if (camPtr->in.arDown) { camPtr->m_pitch += camPtr->m_rotSpeed * delta; }

    camPtr->m_yaw += camPtr->in.m_RX * MOUSE_SENSITIVITY;
    camPtr->m_pitch += camPtr->in.m_RY * MOUSE_SENSITIVITY;

    camPtr->m_pitch = std::clamp(camPtr->m_pitch, -89.0f, 89.0f);

    camPtr->in.m_RX = 0.0f;
    camPtr->in.m_RY = 0.0f;

    camPtr->m_direction = camPtr->m_direction.normalized();

    m_cam->move(delta);
}
