#pragma once

#include <assimp/scene.h>

#include <map>
#include <vector>

namespace fre
{
    struct Material
    {
        Material();
        
        std::map<aiTextureType, uint32_t> mTextureIds;
        float mShininess = 1.0;
        bool hasTextureTypes(const std::vector<aiTextureType>& textureTypes) const;
        uint32_t mId = std::numeric_limits<uint32_t>::max();
        uint32_t mShaderId = std::numeric_limits<uint32_t>::max();
    };
}