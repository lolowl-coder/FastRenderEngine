#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace fre
{
    struct ShaderStageBlob
    {
        std::vector<uint8_t> bytecode;
        std::string entryPoint;
    };
}