#pragma once
#include <iostream>
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

struct Sprite
{
    Sprite() {}
    ~Sprite() { if (tex) SDL_DestroyTexture(tex); }

    Sprite(const Sprite&) = delete;
    Sprite& operator=(const Sprite&) = delete;

    Sprite(Sprite&& o) noexcept : tex(o.tex), src(o.src), dst(o.dst) { o.tex = nullptr; }
    Sprite& operator=(Sprite&& o) noexcept
    {
        if (this != &o)
        {
            if (tex) SDL_DestroyTexture(tex);
            tex = o.tex; src = o.src; dst = o.dst;
            o.tex = nullptr;
        }
        return *this;
    }

    bool loadImage(const std::string& path, SDL_Renderer* SDLRen, bool pixelSave=false)
    {
        SDL_Surface* surf = IMG_Load(path.c_str());
        if (!surf)
        {
            std::cerr << "loadImage() failure: " << SDL_GetError() << ". Likely failed to find: " << path << "\n";
            return false;
        }

        tex = SDL_CreateTextureFromSurface(SDLRen, surf);
        SDL_DestroySurface(surf);
        if (!tex)
        {
           std::cerr << "loadImage() failure: " << SDL_GetError() << ". 'tex' was nullptr " << "\n";
           return false;
        }
        if (pixelSave) { SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST); }

        return true;
    }

    void render(SDL_Renderer* SDLRen)
    {
        SDL_RenderTexture(SDLRen, tex, &src, &dst);
    }

    void clean() { SDL_DestroyTexture(tex); }

    SDL_Texture* tex = nullptr;
    
    SDL_FRect src = {0, 0, 0, 0};
    SDL_FRect dst = {0, 0, 0, 0};
};
