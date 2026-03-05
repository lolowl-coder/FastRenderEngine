#include <fre/IRenderer.hpp>
#include <fre/RendererFactory.hpp>

int main()
{
    auto renderer = fre::createRenderer(fre::RenderAPI::Vulkan);

    fre::RendererConfig desc;
    desc.mEnableValidation = true;
    desc.mHeadless = true;

    if (!renderer->initialize(desc))
        return -1;

    renderer->beginFrame();
    renderer->endFrame();

    renderer->waitIdle();
    renderer->shutdown();

    return 0;
}