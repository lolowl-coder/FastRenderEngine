#pragma once

#include <string>

namespace fre
{
    enum class Requirement
    {
        Optional,
        Required
    };

    struct RequirementRequest
    {
        std::string name;
        Requirement requirement = Requirement::Optional;
        bool enabled = false;
    };

    #define DEFINE_REQUIREMENT(reqName) RequirementRequest reqName = { .name = #reqName }

    bool evaluateRequirement(bool supported, RequirementRequest& requirement);
}