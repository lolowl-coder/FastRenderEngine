#pragma once

#include "fre/core/FileSystemBase.hpp"

#include <map>
#include <string>
#include <vector>

namespace fre
{
	class FileSystemWindows : public FileSystemBase
	{
	public:
		FileSystemWindows();
		virtual void createDirectory(const Path& path) override;
		virtual Path openFileDialog(IWindow& mainWindow, const Path& dir, const char* filter) override;
		virtual Path saveFileDialog(IWindow& mainWindow, const Path& dir, const char* filter) override;
		virtual Path getDocumentsDir() override;
		
	private:
		std::vector<Path> mPath;
	};
}