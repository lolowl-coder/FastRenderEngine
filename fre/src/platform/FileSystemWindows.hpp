#pragma once

#include "fre/core/IFileSystem.hpp"
#include "fre/macros/Member.hpp"

#include <map>
#include <string>
#include <vector>

namespace fre
{
	class FileSystemWindows : public IFileSystem
	{
	public:
		virtual void createDirectory(const Path& path) override;
		virtual Path openFileDialog(const Path& dir, const char* filter) override;
		virtual Path saveFileDialog(const Path& dir, const char* filter) override;
		virtual Path getCurrentDir() override;
		virtual Path getDocumentsDir() override;
		virtual Entries listFiles(const Path& dir) override;
		
	private:
		std::vector<Path> mPath;
	};
}