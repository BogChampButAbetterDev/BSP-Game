#pragma once

#include <iostream>
#include <memory>

#include <SDL3/SDL.h>

#include "globals/globals.hpp"
#include "input.hpp"
#include "camera.hpp"

#include "animation.hpp"

enum class AnimState { HAND_IDLE, SPELL_BASIC };

class Player
{
public:
    Player() {}
    Player(Vector3 pos, SDL_Renderer* SDLRen);

    Camera& getCam() const { return *m_cam; }

    void update(float delta);
    void render();

private:
    SDL_Renderer* m_ren = nullptr;
    std::unique_ptr<Camera> m_cam;

    void initAnims();

    AnimState m_stat = AnimState::HAND_IDLE;
    Animation& currAnim() { return m_stat == AnimState::HAND_IDLE ? handIdle : spellBasic; }

    Animation handIdle;
    Animation spellBasic;
};
