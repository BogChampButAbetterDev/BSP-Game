#pragma once

#include <iostream>
#include <memory>

#include "globals/globals.hpp"
#include "input.hpp"
#include "camera.hpp"

class Player
{
public:
    Player() {}
    Player(Vector3 pos);

    Camera& getCam() const { return *m_cam; }

    void update(float delta);

private:
    std::unique_ptr<Camera> m_cam;
};
