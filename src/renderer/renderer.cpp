#include "renderer.hpp"

void Renderer::beginRender()
{
    m_buf.clear(rgb(0x00, 0x00, 0x00));
}

void Renderer::renderMap(const Map& map)
{
    m_buf.setDrawColor(rgb(0xFF, 0xFF, 0xFF));

    for (const auto& w : map.walls)
    {
        fillWall(&m_buf, w);
    }
}

void Renderer::endRender()
{
    m_buf.present(m_ren);
}
