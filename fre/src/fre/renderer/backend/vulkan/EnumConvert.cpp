#include "fre/renderer/backend/vulkan/EnumConvert.hpp"
#include "fre/core/Log.hpp"

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
        default:
            LOG_ERROR("Unknown FRE format: {}", static_cast<int>(format));
            return vk::Format::eUndefined;
        }
    }

    fre::Format fromVk(vk::Format format)
    {
        switch(format)
        {
        case vk::Format::eUndefined: return Format::Undefined;
        case vk::Format::eR8Unorm: return Format::R8_UNorm;
        case vk::Format::eR8G8Unorm: return Format::RG8_UNorm;
        case vk::Format::eR8G8B8A8Unorm: return Format::RGBA8_UNorm;
        case vk::Format::eB8G8R8A8Unorm: return Format::BGRA8_UNorm;
        case vk::Format::eD16Unorm: return Format::D16;
        case vk::Format::eD24UnormS8Uint: return Format::D24S8;
        case vk::Format::eD32Sfloat: return Format::D32;
        default:
            LOG_ERROR("Unknown Vulkan format: {}", static_cast<int>(format));
            return Format::Undefined;
        }
    }

    vk::ShaderStageFlagBits toVk(ShaderStage stage)
    {
        switch(stage)
        {
        case ShaderStage::Vertex: return vk::ShaderStageFlagBits::eVertex;
        case ShaderStage::Fragment: return vk::ShaderStageFlagBits::eFragment;
        case ShaderStage::Compute: return vk::ShaderStageFlagBits::eCompute;
        };
    }

    vk::ImageUsageFlags toVk(ImageUsage usage)
    {
        vk::ImageUsageFlags flags{};

        if((usage & ImageUsage::TransferSrc) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eTransferSrc;

        if((usage & ImageUsage::TransferDst) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eTransferDst;

        if((usage & ImageUsage::Sampled) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eSampled;

        if((usage & ImageUsage::Storage) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eStorage;

        if((usage & ImageUsage::Color) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eColorAttachment;

        if((usage & ImageUsage::DepthStencil) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eDepthStencilAttachment;

        if((usage & ImageUsage::Input) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eInputAttachment;

        if((usage & ImageUsage::Transient) != ImageUsage::None)
            flags |= vk::ImageUsageFlagBits::eTransientAttachment;

        return flags;
    }

    vk::ComponentSwizzle toVk(ComponentSwizzle s)
    {
        switch(s)
        {
        case ComponentSwizzle::Identity: return vk::ComponentSwizzle::eIdentity;
        case ComponentSwizzle::Zero: return vk::ComponentSwizzle::eZero;
        case ComponentSwizzle::One: return vk::ComponentSwizzle::eOne;
        case ComponentSwizzle::R: return vk::ComponentSwizzle::eR;
        case ComponentSwizzle::G: return vk::ComponentSwizzle::eG;
        case ComponentSwizzle::B: return vk::ComponentSwizzle::eB;
        case ComponentSwizzle::A: return vk::ComponentSwizzle::eA;
        default: return vk::ComponentSwizzle::eIdentity;
        }
    }

    vk::ImageAspectFlagBits toVk(Aspect aspect)
    {
        switch(aspect)
        {
        case Aspect::Color: return vk::ImageAspectFlagBits::eColor;
        case Aspect::Depth: return vk::ImageAspectFlagBits::eDepth;
        case Aspect::Stencil: return vk::ImageAspectFlagBits::eStencil;
        case Aspect::Metadata: return vk::ImageAspectFlagBits::eMetadata;
        case Aspect::Plane0: return vk::ImageAspectFlagBits::ePlane0;
        case Aspect::Plane0KHR: return vk::ImageAspectFlagBits::ePlane0KHR;
        case Aspect::Plane1: return vk::ImageAspectFlagBits::ePlane1;
        case Aspect::Plane1KHR: return vk::ImageAspectFlagBits::ePlane1KHR;
        case Aspect::Plane2: return vk::ImageAspectFlagBits::ePlane2;
        case Aspect::Plane2KHR: return vk::ImageAspectFlagBits::ePlane2KHR;
        case Aspect::None: return vk::ImageAspectFlagBits::eNone;
        case Aspect::NoneKHR: return vk::ImageAspectFlagBits::eNoneKHR;
        case Aspect::MemoryPlane0EXT: return vk::ImageAspectFlagBits::eMemoryPlane0EXT;
        case Aspect::MemoryPlane1EXT: return vk::ImageAspectFlagBits::eMemoryPlane1EXT;
        case Aspect::MemoryPlane2EXT: return vk::ImageAspectFlagBits::eMemoryPlane2EXT;
        case Aspect::MemoryPlane3EXT: return vk::ImageAspectFlagBits::eMemoryPlane3EXT;
        default: return vk::ImageAspectFlagBits::eNone;
        }
    }

    vk::SampleCountFlagBits toVk(SampleCount count)
    {
        switch(count)
        {
        case SampleCount::Sample1: return vk::SampleCountFlagBits::e1;
        case SampleCount::Sample2: return vk::SampleCountFlagBits::e2;
        case SampleCount::Sample4: return vk::SampleCountFlagBits::e4;
        case SampleCount::Sample8: return vk::SampleCountFlagBits::e8;
        case SampleCount::Sample16: return vk::SampleCountFlagBits::e16;
        case SampleCount::Sample32: return vk::SampleCountFlagBits::e32;
        case SampleCount::Sample64: return vk::SampleCountFlagBits::e64;
        default: return vk::SampleCountFlagBits::e1;
        }
	}

    vk::Viewport toVk(const Viewport& viewport)
    {
        vk::Viewport vkViewport{};
        vkViewport.x = viewport.x;
        vkViewport.y = viewport.y;
        vkViewport.width = viewport.width;
        vkViewport.height = viewport.height;
        vkViewport.minDepth = viewport.minDepth;
        vkViewport.maxDepth = viewport.maxDepth;
        return vkViewport;
    }
    vk::Rect2D toVk(const ScissorRect& scissorRect)
    {
        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D(static_cast<int32_t>(scissorRect.x), static_cast<int32_t>(scissorRect.y));
        scissor.extent = vk::Extent2D(static_cast<uint32_t>(scissorRect.width), static_cast<uint32_t>(scissorRect.height));
        return scissor;
	}
}