#pragma once

#include "fre/core/Pointers.hpp"
#include "fre/renderer/IGpuImage.hpp"

namespace fre
{
    FileSystemPtr createFileSystem();
    WindowManagerPtr createWindowManager();
}