#include "player.hpp"

Player::Player(Vector3 pos, SDL_Renderer* ren)
{
    m_cam = std::make_unique<Camera>(pos);
    m_ren = ren;

    initAnims();
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

    if (camPtr->in.lmb)
    {
        if (m_stat != AnimState::SPELL_BASIC)
        {
            m_stat = AnimState::SPELL_BASIC;
            spellBasic.reset();
        }
    }

    if (m_stat == AnimState::SPELL_BASIC && currAnim().isFinished())
    {
        m_stat = AnimState::HAND_IDLE;
        handIdle.reset();
    }

    currAnim().update(delta);
}

void Player::render()
{
    currAnim().renderFrame(m_ren, 585, 420);
}

void Player::initAnims()
{
    handIdle.setImage("assets/hands/weaponspell1.png", m_ren);
    handIdle.setRenderScale(5.5);
    handIdle.setFrameTime(1.7);
    handIdle.setLooping(true);
    handIdle.addFrame({ 48, 2, 45, 36 });

    spellBasic.setImage("assets/hands/weaponspell1.png", m_ren);
    spellBasic.setRenderScale(5.5);
    spellBasic.setFrameTime(0.1f);
    spellBasic.setLooping(false);
    spellBasic.addFrame({ 48, 2, 45, 36 });
    spellBasic.addFrame({ 10, 45, 47, 41 });
    spellBasic.addFrame({ 10, 45, 47, 41 });
    spellBasic.addFrame({ 96, 41, 49, 44 });
}
