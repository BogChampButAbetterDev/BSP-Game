#include "renderer.hpp"

void Renderer::beginRender()
{
    m_buf.clear(0x000000FF);
}

void Renderer::renderLoop()
{
    m_buf.setDrawColor(0xFF0000FF);
}

void Renderer::endRender()
{
    m_buf.present(m_ren);
}
