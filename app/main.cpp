#include <fre/IRenderer.hpp>
#include <fre/RendererFactory.hpp>

int main()
{
    auto renderer = fre::createRenderer(fre::RenderAPI::Vulkan);

    fre::RendererConfig config;
    config.mEnableValidation = true;
    config.mHeadless = true;
    config.mGpuSelectionMode = fre::GPUSelectionMode::Auto;
    config.mWidth = 1920;
    config.mHeight = 1080;
    config.mFeatures.dynamicRendering.requirement = fre::FeatureRequirement::Required;
    config.mFeatures.timelineSemaphore.requirement = fre::FeatureRequirement::Optional;
    config.mFeatures.bufferDeviceAddress.requirement = fre::FeatureRequirement::Optional;
    config.mFeatures.descriptorIndexing.requirement = fre::FeatureRequirement::Optional;
    config.mFeatures.synchronization2.requirement = fre::FeatureRequirement::Optional;
    config.mFeatures.accelerationStructure.requirement = fre::FeatureRequirement::Optional;
    config.mFeatures.rayTracingPipeline.requirement = fre::FeatureRequirement::Optional;
    config.mFeatures.rayQuery.requirement = fre::FeatureRequirement::Optional;

    if (!renderer->initialize(config))
        return -1;

    renderer->beginFrame();
    renderer->endFrame();

    renderer->waitIdle();
    renderer->shutdown();

    return 0;
}