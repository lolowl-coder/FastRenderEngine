#pragma once

#include <string>

namespace fre
{
    class IShader
    {
    public:
        virtual ~IShader() = default;

        virtual const std::string& getName() const = 0;
    };
}