#pragma once

#include <glm/glm.hpp>

namespace fre
{
    struct Light
    {
        glm::vec3 mPosition = glm::vec3(100.0f);
        glm::vec3 mColor = glm::vec3(1.0f);
    };
}