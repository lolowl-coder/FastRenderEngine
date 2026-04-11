#include "fre/core/Format.hpp"

namespace fre
{
    vk::Format toVk(fre::Format format)
    {
        switch(format)
        {
            case Format::Undefined: return vk::Format::eUndefined;
            case Format::R8_UNorm: return vk::Format::eR8Unorm;
            case Format::RG8_UNorm: return vk::Format::eR8G8Unorm;
            case Format::RGBA8_UNorm: return vk::Format::eR8G8B8A8Unorm;
            case Format::BGRA8_UNorm: return vk::Format::eB8G8R8A8Unorm;
            case Format::D16: return vk::Format::eD16Unorm;
            case Format::D24S8: return vk::Format::eD24UnormS8Uint;
            case Format::D32: return vk::Format::eD32Sfloat;
            default: return vk::Format::eUndefined;
        }
    }
}