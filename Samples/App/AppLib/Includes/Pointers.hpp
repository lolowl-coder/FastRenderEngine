#pragma once

#include <memory>

namespace fre
{
	struct VulkanBuffer;

    using VulkanBufferPtr = std::shared_ptr<VulkanBuffer>;
}