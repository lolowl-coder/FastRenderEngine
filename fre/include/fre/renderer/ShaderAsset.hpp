#pragma once

#include "fre/renderer/ShaderStage.hpp"
#include "fre/renderer/ShaderStageBlob.hpp"

#include <unordered_map>

namespace fre
{
    struct ShaderAsset
    {
        std::string name;

        std::unordered_map<ShaderStage, ShaderStageBlob> blobs;
        ShaderPtr shader;
    };
}