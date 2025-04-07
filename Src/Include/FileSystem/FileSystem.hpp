#pragma once

#include "Macros/Member.hpp"

#include <map>
#include <string>
#include <vector>

#define FS auto& fs = fre::FileSystem::getInstance()

namespace fre
{
	class FileSystem
	{
	public:
		using Entries = std::vector<std::string>;

		FileSystem();
		static FileSystem& getInstance();

		void addPath(const std::string& path) { mPath.push_back(path); }
		std::string openFileDialog(const char* dir, const char* filter);
		std::string saveFileDialog(const char* dir, const char* filter);
		std::string getCurrentDir();
		std::string getDocumentsDir();
		Entries listFiles(const std::string& dir);

		std::string find(const std::string& fileName);
		std::string trim(const std::string& path);
		std::string getExt(const std::string& fileName);
		FIELD_NS(std::string, AppDir, private, public, public);
	private:
		std::vector<std::string> mPath;
	};
}