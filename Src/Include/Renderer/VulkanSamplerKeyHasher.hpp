#pragma once

#include "Renderer/VulkanSampler.hpp"

#include <functional>

namespace std
{
    template <>
    struct hash<fre::VulkanSamplerKey>
    {
        std::size_t operator()(const fre::VulkanSamplerKey& key) const
        {
            std::size_t seed = 0;
            std::hash<uint32_t> hasher;
            seed ^= hasher(key.mAddressMode) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hasher(key.mFilter) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            seed ^= hasher(key.mUnnormalizedCoordinates) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

            return seed;
        }
    };
}