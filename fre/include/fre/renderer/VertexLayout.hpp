#pragma once

#include <vector>

namespace fre
{
    enum class VertexFormat
    {
        Float1,
        Float2,
        Float3,
        Float4,
        UInt,
        Int
    };

    struct VertexAttribute
    {
        // Shader location
        uint32_t location = 0;
        VertexFormat format;
        uint32_t offset = 0;
    };

    struct VertexBufferBinding
    {
        uint32_t binding = 0;
        uint32_t stride = 0;
    };

    struct VertexLayout
    {
        std::vector<VertexAttribute> attributes;
        std::vector<VertexBufferBinding> bindings;

        bool isEmpty() const
        {
            return attributes.empty();
        }
    };
}