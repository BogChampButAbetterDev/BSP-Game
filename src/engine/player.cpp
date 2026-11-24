#include "player.hpp"

static constexpr float GRAVITY       = 25.0f;
static constexpr float STEP_UP_SPEED = 15.0f;
static constexpr float JUMP_STRENGTH = 8.0f;

inline constexpr SDL_FRect HAND_IDLE_FRAME = { 48, 2, 45, 36 };
inline constexpr float HAND_RENDER_SCALE   = 5.5f;

inline constexpr float HAND_OVERHANG_RIGHT  = 32.5f;
inline constexpr float HAND_OVERHANG_BOTTOM = 1.0f;

inline constexpr float HAND_ANCHOR_X = DEF_WIN_WIDTH  + HAND_OVERHANG_RIGHT;
inline constexpr float HAND_ANCHOR_Y = DEF_WIN_HEIGHT + HAND_OVERHANG_BOTTOM;

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
        m_feetY = s->floorAt({m_cam->m_pos.x, m_cam->m_pos.z});
        m_velY = 0.0f;
        m_grounded = true;
        m_cam->m_pos.y = m_feetY + m_cam->getEyeHeight();
    }
}

void Player::updateVertical(float delta)
{
    if (!m_world) return;
    Vector2 pp{m_cam->m_pos.x, m_cam->m_pos.z};
    const Sector* s = m_world->sectorAt(pp);
    if (!s) return;

    float fl = s->floorAt(pp), cl = s->ceilAt(pp);
    float prev = m_feetY;
    bool wasGrounded = m_grounded;

    m_velY -= GRAVITY * delta;
    m_feetY += m_velY * delta;

    if (wasGrounded && m_velY <= 0.0f && m_feetY > fl && m_feetY - fl < m_col.stepHeight)
    {
        m_feetY = fl;
        m_velY = 0.0f;
    }

    if (m_feetY + m_col.height > cl)
    {
        m_feetY = cl - m_col.height;
        if (m_velY > 0.0f) m_velY = 0.0f;
    }

    if (m_feetY < fl)
    {
        if (prev >= fl - 0.01f) m_feetY = fl;
        else m_feetY = std::min(fl, prev + STEP_UP_SPEED * delta);
        m_velY = 0.0f;
    }

    m_grounded = m_feetY <= fl + 0.01f;
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

        bool jumpPressed = camPtr->in.space && !m_prevJump;
        m_prevJump = camPtr->in.space;
        if (jumpPressed && m_grounded) 
        {
            m_velY = JUMP_STRENGTH; m_grounded = false;
        }

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
    currAnim().renderFrameAnchored(m_ren, HAND_ANCHOR_X, HAND_ANCHOR_Y);
}

void Player::initAnims()
{
    handIdle.setImage("assets/hands/weaponspell1.png", m_ren);
    handIdle.setRenderScale(HAND_RENDER_SCALE);
    handIdle.setFrameTime(1.7);
    handIdle.setLooping(true);
    handIdle.addFrame(HAND_IDLE_FRAME);

    spellBasic.setImage("assets/hands/weaponspell1.png", m_ren);
    spellBasic.setRenderScale(HAND_RENDER_SCALE);
    spellBasic.setFrameTime(0.1f);
    spellBasic.setLooping(false);
    spellBasic.addFrame({ 48, 2, 45, 36 });
    spellBasic.addFrame({ 10, 45, 47, 41 });
    spellBasic.addFrame({ 10, 45, 47, 41 });
    spellBasic.addFrame({ 96, 41, 49, 44 });
}
