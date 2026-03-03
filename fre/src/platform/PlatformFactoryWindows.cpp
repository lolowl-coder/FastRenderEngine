#pragma once

#include "fre/core/PlatformFactory.hpp"
#include "platform/FileSystem.hpp"

namespace fre
{
    FileSystemPtr createFileSystem()
    {
        return std::make_unique<FileSystemWindows>();
    }
}