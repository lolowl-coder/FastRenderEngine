#pragma once

#include <cstdint>

namespace fre
{
    struct TextureHandle
    {
        uint32_t index;
        uint32_t generation;
    };
}