#pragma once

namespace fre
{
    struct Extent2D
    {
        uint32_t width = {};
        uint32_t height = {};
    };

    struct ScissorRect
    {
        int32_t x = {};
        int32_t y = {};

        uint32_t width = {};
        uint32_t height = {};
    };

    struct Viewport
    {
        float x = {};
        float y = {};
        float width = {};
        float height = {};

        float minDepth = 0.0f;
        float maxDepth = 1.0f;
    };
}