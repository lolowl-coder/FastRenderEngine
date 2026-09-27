#pragma once

#include <string>
#include "fre/renderer/VertexLayout.hpp"

namespace fre
{
    class IShader
    {
    public:
        virtual ~IShader() = default;

        virtual const std::string& getName() const = 0;
        virtual const VertexLayout& getVertexLayout() const = 0;
    };
}