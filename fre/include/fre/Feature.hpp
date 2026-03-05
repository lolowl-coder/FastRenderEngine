#pragma once

namespace fre
{
    enum class FeatureRequirement
    {
        Optional,
        Required
    };

    struct FeatureRequest
    {
        std::string name;
        FeatureRequirement requirement = FeatureRequirement::Optional;
        bool enabled = false;
    };

    #define DEFINE_FEATURE(featureName) FeatureRequest featureName = { .name = #featureName }

    struct RendererFeatureConfig
    {
		DEFINE_FEATURE(dynamicRendering);
        DEFINE_FEATURE(timelineSemaphore);
        DEFINE_FEATURE(descriptorIndexing);
        DEFINE_FEATURE(bufferDeviceAddress);
        DEFINE_FEATURE(synchronization2);
        DEFINE_FEATURE(accelerationStructure);
        DEFINE_FEATURE(rayTracingPipeline);
        DEFINE_FEATURE(rayQuery);
    };
}