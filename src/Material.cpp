#include "Material.hpp"

namespace fre
{
    Material::Material()
    {
        static uint32_t materialId = 0;
        mId = materialId++;
    }

    bool Material::hasTextureTypes(const std::vector<aiTextureType>& textureTypes) const
    {
        bool result = true;
        for(const auto& textureType : textureTypes)
        {
            const auto& foundIt = mTextureIds.find(textureType);
            if(foundIt == mTextureIds.end())
            {
                result = false;
                break;
            }
        }
        return result;
    }
}