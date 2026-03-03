#include "fre/RendererFactory.hpp"
#include "Vulkan/VulkanRenderer.hpp"

namespace fre
{
    std::unique_ptr<IRenderer> createRenderer(RenderAPI api)
    {
        switch (api)
        {
            case RenderAPI::Vulkan:
                return std::make_unique<VulkanRenderer>();
        }
        return nullptr;
    }
}