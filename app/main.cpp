#include <fre/IRenderer.hpp>
#include <fre/RendererFactory.hpp>
#include <fre/RendererDesc.hpp>

int main()
{
    auto renderer = fre::createRenderer(fre::RenderAPI::Vulkan);

    fre::RendererDesc desc;
    desc.enableValidation = true;
    desc.headless = true;

    if (!renderer->initialize(desc))
        return -1;

    renderer->beginFrame();
    renderer->endFrame();

    renderer->waitIdle();
    renderer->shutdown();

    return 0;
}