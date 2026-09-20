#include "game.hpp"

bool Game::init()
{
    if (!Platform::init()) { return false; }

    m_win = Platform::create_window(DEF_WIN_WIDTH, DEF_WIN_HEIGHT);
    if (!m_win) { return false; }

    m_sdlren = Platform::create_renderer(m_win);
    if (!m_sdlren) 
    { 
        if (m_win) 
        {
            Platform::destroy_window(m_win);
        } 
        return false; 
    }

    m_ren = Renderer(m_sdlren);

    lastTime = SDL_GetTicks();

    return true;
}

void Game::run()
{
    m_ml = MapLoader("maps/test.lvl");
    m_map = m_ml.read();

    m_cam = Camera({0, 0, 0});
    m_map.onLoad(m_cam);

    m_ren.getBuf()->setActiveCamera(&m_cam);

    Uint32 frameCount = 0;
    Uint64 lastFpsUpdateTime = SDL_GetTicks(); 
    char title_buf[64];

    while (m_running)
    {
        frameCount++;

        Uint64 currentTime = SDL_GetTicks();
        delta = (float)(currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        if (currentTime - lastFpsUpdateTime >= 1000)
        {
            snprintf(title_buf, sizeof(title_buf), "BSP Renderer | FPS: %u", frameCount);
            SDL_SetWindowTitle(m_win, title_buf);

            frameCount = 0; 
            lastFpsUpdateTime = currentTime; 
        }
        
        pollEvents();

        m_cam.update(delta);

        SDL_SetRenderDrawColor(m_sdlren, 0, 0, 0, 255);
        SDL_RenderClear(m_sdlren);
        m_ren.beginRender();

        m_ren.renderMap(m_map);

        m_ren.endRender();
        SDL_RenderPresent(m_sdlren);
    }
}

void Game::pollEvents()
{
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                m_running = false;
                break;
        }

        if ((event.type == SDL_EVENT_KEY_DOWN) && (event.key.scancode == SDL_SCANCODE_ESCAPE))
        {
            m_running = false;
        }

        pollInput(event, &m_cam.in);
    }
}

void Game::end()
{
    Platform::destroy_renderer(m_sdlren);
    Platform::destroy_window(m_win);
    Platform::shutdown();
}
