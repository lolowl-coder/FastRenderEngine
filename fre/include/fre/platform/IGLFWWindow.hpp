#pragma once

struct GLFWwindow;

#include "fre/core/Requirement.hpp"
#include <vector>

namespace fre
{
    class IGLFWWindow
    {
    public:
        virtual GLFWwindow* getGLFWwindow() const = 0;
        virtual std::vector<RequirementRequest> getInstanceExtensions() = 0;
    };
}