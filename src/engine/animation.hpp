#pragma once
#include <vector>
#include <SDL3/SDL.h>

#include "sprite.hpp"

struct Animation
{
    Animation() {}

    void setImage(const std::string& path, SDL_Renderer* SDLRen) 
    { 
        if (!m_sprite.loadImage(path, SDLRen, true))  
        {
            m_sprite.clean();
        }
    }

    void setFrameTime(float time)
    {
        m_frameTime = time;
    }

    void addFrame(SDL_FRect src)
    {
        m_frames.push_back(src);
    }

    void setLooping(bool val) { m_loops = val; }

    void reset() { m_currentFrame = 0; m_timer = 0.0f; } 
    bool isFinished() const { return m_currentFrame >= (int)m_frames.size() - 1 || m_timer >= m_frameTime; }

    void update(float delta)
    {
        if (m_frameTime == 0.0f || m_frames.empty()) return;
        if (!m_loops && m_currentFrame >= (int)m_frames.size() - 1) return;

        m_timer += delta;
        if (m_timer >= m_frameTime)
        {
            m_timer = 0.0f;
            m_currentFrame++;

            if (m_currentFrame >= (int)m_frames.size())
            {
                if (m_loops) { m_currentFrame = 0; }
                else { m_currentFrame = (int)m_frames.size() - 1; }
            }
        }
    }

    void renderFrame(SDL_Renderer* SDLRen, float x, float y)
    {
        if (m_frames.empty()) return;
        m_sprite.src = getCurrentFrame();
        m_sprite.dst = {x, y, m_sprite.src.w * m_spriteScale, m_sprite.src.h * m_spriteScale};

        m_sprite.render(SDLRen);
    }

    SDL_FRect getCurrentFrame() 
    {
        if (m_frames.empty()) return {0, 0, 0, 0};
        if (m_currentFrame < 0) 
        {
            m_currentFrame = 0;
        }
        else if (m_currentFrame >= (int)m_frames.size()) 
        {
            m_currentFrame = (int)m_frames.size() - 1;
        }

        return m_frames[m_currentFrame];
    }

    void setRenderScale(float val) { m_spriteScale = val; }

private:
    std::vector<SDL_FRect> m_frames;

    Sprite m_sprite;

    int m_currentFrame = 0;

    float m_frameTime = 0.0f;
    float m_timer = 0.0f;

    float m_spriteScale = 1.0f;
    
    bool m_loops = false;
};
