#pragma once

#include 

namespace fre
{
    class IShaderManager
    {
    public:
        virtual ~IShaderManager() = default;
        virtual IShader* load(const std::string& name) = 0;
    };
}