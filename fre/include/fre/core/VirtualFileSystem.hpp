#pragma once

#include "fre/core/IFileSystem.hpp"
#include "fre/macros/Member.hpp"

#include <vector>

namespace fre
{
    class VirtualFileSystem
    {
    public:
        using Path = std::filesystem::path;

        explicit VirtualFileSystem(IFileSystem& fs);

		// Add search path
        void mount(const Path& path);
        void unmount(const Path& path);

		// Search across mounts
        Path find(const Path& fileName) const;

        std::vector<Path> mounts() const;

    private:
        IFileSystem& mFS;
        std::vector<Path> mMounts;
    };

	using VirtualFileSystemPtr = std::unique_ptr<VirtualFileSystem>;
}