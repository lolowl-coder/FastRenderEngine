#pragma once

#include <assimp/scene.h>

#include <map>

namespace fre
{
    struct Material
    {
        std::map<aiTextureType, uint32_t> mTextureIds;
    };
}