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

    std::cout << "walls loaded: " << m_map.walls.size() << "\n";
    if (!m_map.walls.empty())
    {
        Wall& w = m_map.walls[0];
        std::cout << "wall[0]: (" << w.line.start.x << "," << w.line.start.y << ") -> ("
                << w.line.end.x << "," << w.line.end.y << ") bottom=" << w.bottom << " top=" << w.top << "\n";
    }
    std::cout << "cam pos after onLoad: (" << m_cam.getPos().x << ", " << m_cam.getPos().y << ", " << m_cam.getPos().z << ")\n";

    while (m_running)
    {
        Uint64 currentTime = SDL_GetTicks();
        delta = (float)(currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        
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
