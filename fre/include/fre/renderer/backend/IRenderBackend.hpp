#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/renderer/IGpuImage.hpp"
#include "fre/renderer/IGpuImageView.hpp"

namespace fre
{
    class IRenderBackend
    {
    public:
        virtual ~IRenderBackend() = default;
        virtual bool initialize() = 0;
        virtual void shutdown() = 0;
        virtual void waitIdle() = 0 ;
    };
}