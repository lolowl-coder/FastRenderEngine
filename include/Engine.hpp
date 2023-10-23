#pragma once

#include <cstdint>

namespace fre
{
    class Engine
    {
    public:
        void init(uint16_t width, uint16_t height);
        void destroy();
    };
}