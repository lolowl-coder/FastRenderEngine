#pragma once

#include "fre/renderer/backend/vulkan/VulkanCommon.hpp"

namespace fre
{
    template<typename T>
    T vkCheck(vk::ResultValue<T>&& rv)
    {
        if(rv.result != vk::Result::eSuccess)
            throw std::runtime_error("Vulkan error: " + vk::to_string(rv.result));

        return std::move(rv.value);
    }

    void vkCheck(vk::Result result);
}