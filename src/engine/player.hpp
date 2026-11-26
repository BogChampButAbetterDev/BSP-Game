#pragma once

#include <iostream>
#include <memory>

#include <SDL3/SDL.h>

#include "collision.hpp"
#include "raycast.hpp"
#include "camera.hpp"

#include "animation.hpp"

enum class AnimState { HAND_IDLE, SPELL_BASIC };

class Player
{
public:
    Player() {}
    Player(Vector3 pos, SDL_Renderer* SDLRen);

    Camera& getCam() const { return *m_cam; }

    void setWorld(const CollisionWorld* w) { m_world = w; }
    void setRayWorld(const RayWorld* r)    { m_rayWorld = r; }

    const std::vector<DebugTrace>& getTraces() const { return m_traces; }

    void update(float delta);
    void render();

    void snapToFloor();

private:
    SDL_Renderer* m_ren = nullptr;
    std::unique_ptr<Camera> m_cam;

    const CollisionWorld* m_world = nullptr;
    CollisionParams m_col;

    const RayWorld* m_rayWorld = nullptr;
    std::vector<DebugTrace> m_traces;

    void fireTrace();

    void updateVertical(float delta);

    float m_feetY = 0.0f;
    Vector2 m_vel = { 0.0f, 0.0f };
    float m_velY = 0.0f;
    bool  m_grounded = false;
    bool m_prevJump = false;

    void initAnims();

    AnimState m_stat = AnimState::HAND_IDLE;
    Animation& currAnim() { return m_stat == AnimState::HAND_IDLE ? handIdle : spellBasic; }

    Animation handIdle;
    Animation spellBasic;
    Animation crosshair;
};
