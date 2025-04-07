#pragma once

namespace fre
{
    struct FeatureStorageBase {
        virtual ~FeatureStorageBase() = default;
        virtual void* get() = 0;
    };

    template <typename T>
    struct FeatureStorage : FeatureStorageBase {
        T data;
        FeatureStorage() { memset(&data, 0, sizeof(T)); }
        void* get() override { return &data; }
    };
}