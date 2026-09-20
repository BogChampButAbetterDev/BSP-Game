#include "renderer.hpp"

void Renderer::beginRender()
{
    m_buf.clear(0x000000FF);
}

void Renderer::renderMap(const Map& map)
{
    m_buf.setDrawColor(0xFFFFFFFF);

    for (const auto w : map.walls)
    {
        fillWall(&m_buf, w);
    }
}

void Renderer::endRender()
{
    m_buf.present(m_ren);
}
