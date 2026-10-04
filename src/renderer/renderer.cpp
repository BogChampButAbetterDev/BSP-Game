#include "renderer.hpp"

void Renderer::beginRender()
{
    m_buf.clear(rgb(0x00, 0x00, 0x00));
}

void Renderer::renderMap(const Map& map)
{
    Vector2 camFloorPos = {m_buf.activeCamera->getPos().x, m_buf.activeCamera->getPos().z};
    m_renOrder->clear();

    BSPTraverse(map.tree, map.tree.root, camFloorPos, *m_renOrder);
    for (const Wall& w : (*m_renOrder))
    {
        if (w.cullWall(camFloorPos)) continue;
        m_buf.setDrawColor(rgb(165, 182, 127));
        fillWall(&m_buf, w);
    }
}

void Renderer::renderMapDBGCLR(const Map& map)
{
    Vector2 camFloorPos = {m_buf.activeCamera->getPos().x, m_buf.activeCamera->getPos().z};

    m_renOrder->clear();

    BSPTraverse(map.tree, map.tree.root, camFloorPos, *m_renOrder);
    for (size_t i = 0; i < m_renOrder->size(); i++)
    {
        if ((*m_renOrder)[i].cullWall(camFloorPos)) continue;
        m_buf.setDrawColor(debugColor(i));
        fillWall(&m_buf, (*m_renOrder)[i]);
    }
}

void Renderer::render2DView(const Map &map)
{
    DebugView::render(&m_buf, map.walls);
}

void Renderer::endRender()
{
    m_buf.present(m_ren);
}
