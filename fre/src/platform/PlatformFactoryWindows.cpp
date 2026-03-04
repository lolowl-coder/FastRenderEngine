#pragma once

#include "fre/core/PlatformFactory.hpp"
#include "platform/FileSystemWindows.hpp"

namespace fre
{
    FileSystemPtr createFileSystem()
    {
        return std::make_unique<FileSystemWindows>();
    }
}