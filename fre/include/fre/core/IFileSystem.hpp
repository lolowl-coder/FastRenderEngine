#pragma once

#include <filesystem>
#include <vector>
#include <memory>

namespace fre
{
    class IFileSystem
    {
    public:
        using Path = std::filesystem::path;
        using Entries = std::vector<Path>;

        virtual ~IFileSystem() = default;

		virtual void createDirectory(const Path& path) = 0;

        virtual Path openFileDialog(const Path& dir, const char* filter) = 0;
        virtual Path saveFileDialog(const Path& dir, const char* filter) = 0;

        virtual Path getCurrentDir() = 0;
        virtual Path getDocumentsDir() = 0;

        virtual bool exists(const Path& path) = 0;
        virtual Entries listFiles(const Path& dir) = 0;
    };

    using FileSystemPtr = std::unique_ptr<IFileSystem>;
}