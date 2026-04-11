#include "fre/core/FileSystemBase.hpp"

namespace fre
{
	FileSystemBase::FileSystemBase()
	{
		mAppDir = getCurrentDir();
	}

	IFileSystem::Path FileSystemBase::getCurrentDir()
	{
		return std::filesystem::current_path();
	}

	FileSystemBase::Path FileSystemBase::trim(const Path& path)
	{
		auto tmp = path.generic_string();
		auto sub = mAppDir.generic_string();
		size_t pos = tmp.find(sub);
		if (pos != std::string::npos)
		{
			tmp.erase(pos, sub.length() + 1);
		}
		return tmp;
	}

	FileSystemBase::Path FileSystemBase::getExt(const Path& fileName)
	{
		return fileName.extension();
	}

	bool FileSystemBase::exists(const Path& path)
	{
		return std::filesystem::exists(path);
	}

	FileSystemBase::Entries FileSystemBase::listFiles(const Path& dir)
	{
		Entries result;
		for (auto const& entry : std::filesystem::directory_iterator{ dir })
		{
			result.push_back(entry.path().filename().generic_string());
		}

		return result;
	}
}