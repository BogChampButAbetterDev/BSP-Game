#include "renderer.hpp"

void Renderer::beginRender()
{
    m_buf.clear(0x000000FF);
}

void Renderer::renderLoop()
{
    m_buf.setDrawColor(0xFFFFFFFF);
}

void Renderer::endRender()
{
    m_buf.present(m_ren);
}
