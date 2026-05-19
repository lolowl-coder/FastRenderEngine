#pragma once

#include "fre/renderer/ShaderStage.hpp"

namespace fre
{
    struct ShaderLoadDesc
    {
        std::string name;

        struct StageDesc
        {
            ShaderStage stage;
            std::string path;
            std::string entryPoint;
        };

        std::vector<StageDesc> stages;
    };
}