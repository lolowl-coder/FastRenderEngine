#pragma once

#include <memory>

namespace fre
{
	class IFileSystem;
	class VirtualFileSystem;

	using FileSystemPtr = std::unique_ptr<IFileSystem>;
	using VirtualFileSystemPtr = std::unique_ptr<VirtualFileSystem>;
}