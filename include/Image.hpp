#pragma once

#include "stb_image.h"

#include <glm/glm.hpp>

#include <string>
#include <limits>

namespace fre
{
    struct Image
    {
        void load(const std::string& path);
        void destroy();

        uint32_t mId = std::numeric_limits<uint32_t>::max();
        glm::ivec2 mDimension;
        uint32_t mDataSize = 0;
        stbi_uc* mData = nullptr;
    };
}