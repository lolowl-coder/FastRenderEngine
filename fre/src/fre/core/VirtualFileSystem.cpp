#include "fre/core/VirtualFileSystem.hpp"

namespace fre
{
	VirtualFileSystem::VirtualFileSystem(IFileSystem& fs)
		: mFS(fs)
	{
		auto docDir = mFS.getDocumentsDir();
		if(!mFS.exists(Path(docDir)))
		{
			mFS.createDirectory(docDir);
		}
	}

	void VirtualFileSystem::mount(const Path& path)
	{
		mMounts.push_back(path);
	}
	
	void VirtualFileSystem::unmount(const Path& path)
	{
		auto found = std::find(mMounts.begin(), mMounts.end(), path);
		if(found != mMounts.end())
		{
			mMounts.erase(found);
		}
	}

	VirtualFileSystem::Path VirtualFileSystem::find(const Path& fileName) const
	{
		Path result;
		for(auto m : mMounts)
		{
			auto currentPath = m / fileName;
			if(mFS.exists(currentPath))
			{
				result = currentPath;
			}
		}
		return result;
	}

	std::vector<IFileSystem::Path> VirtualFileSystem::mounts() const
	{
		return mMounts;
	}

	std::vector<uint8_t> VirtualFileSystem::readFile(const Path& fileName) const
	{
		auto path = find(fileName);
		if(path.empty())
		{
			throw std::runtime_error("File not found: " + fileName.string());
		}
		return mFS.readFile(path);
	}
}