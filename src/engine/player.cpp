#include "player.hpp"

static constexpr float GRAVITY       = 25.0f;
static constexpr float STEP_UP_SPEED = 15.0f;

Player::Player(Vector3 pos, SDL_Renderer* ren)
{
    m_cam = std::make_unique<Camera>(pos);
    m_ren = ren;

    initAnims();
}

void Player::snapToFloor()
{
    if (!m_world) return;
    if (const Sector* s = m_world->sectorAt({m_cam->m_pos.x, m_cam->m_pos.z}))
    {
        m_feetY = s->floorHt;
        m_velY = 0.0f;
        m_grounded = true;
        m_cam->m_pos.y = m_feetY + m_cam->getEyeHeight();
    }
}

void Player::updateVertical(float delta)
{
    if (!m_world) return;

    const Sector* s = m_world->sectorAt({m_cam->m_pos.x, m_cam->m_pos.z});
    if (!s) return;

    float prev = m_feetY;

    m_velY -= GRAVITY * delta;
    m_feetY += m_velY * delta;

    if (m_feetY + m_col.height > s->ceilingHt)
    {
        m_feetY = s->ceilingHt - m_col.height;
        if (m_velY > 0.0f) m_velY = 0.0f;
    }

    if (m_feetY < s->floorHt)
    {
        if (prev >= s->floorHt - 0.01f)
            m_feetY = s->floorHt;
        else
            m_feetY = std::min(s->floorHt, prev + STEP_UP_SPEED * delta); 
        m_velY = 0.0f;
    }

    m_grounded = m_feetY <= s->floorHt + 0.01f;
    m_cam->m_pos.y = m_feetY + m_cam->getEyeHeight();
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

    Vector3 d = m_cam->computeMove(delta);
    if (m_world)
    {
        Vector2 p = m_world->move({m_cam->m_pos.x, m_cam->m_pos.z}, {d.x, d.z}, m_feetY, m_col);
        m_cam->m_pos.x = p.x;
        m_cam->m_pos.z = p.y;
        updateVertical(delta);
    }
    else
    {
        m_cam->m_pos += d;
    }

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
    currAnim().renderFrame(m_ren, 585, 403);
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
