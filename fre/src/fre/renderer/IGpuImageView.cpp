#include "fre/renderer/IGpuImageView.hpp"

namespace fre
{
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
}